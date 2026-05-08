#include "Handlers/NextionHandler.h"
#include "Handlers/NextionComponents.h"
#include "Handlers/LogHandler.h"
#include "Handlers/FileSystem.h"
#include "SysStatMutex.h"

extern LogHandler logHandler;

#define nexSerial Serial2

// State tracking for change detection
static float lastMinBBQTemp   = 0;
static float lastMaxBBQTemp   = 0;
static float lastMinChunkTemp = 0;
static float lastMaxChunkTemp = 0;
static float lastMinCaliBBQ   = 0;
static float lastMaxCaliBBQ   = 0;
static float lastMinCaliChunk = 0;
static float lastMaxCaliChunk = 0;

static uint32_t lastPageIdBBQ   = NEXTION_PAGE_NONE;
static uint32_t lastPageIdChunk = NEXTION_PAGE_NONE;
static uint32_t lastPageIdCali  = NEXTION_PAGE_NONE;

static bool initialUpdateDoneBBQ   = false;
static bool initialUpdateDoneChunk = false;
static bool initialUpdateDoneCali  = false;

static struct {
    int lastBBQTemp     = -999;
    int lastProbeTemp   = -999;
    int lastBBQTarget   = -999;
    int lastProbeTarget = -999;
    int lastAvgTemp     = -999;
    bool lastRelayState = false;
    uint32_t lastPageId = NEXTION_PAGE_NONE;
    unsigned long lastUpdate = 0;
} nexCache;

// --- Callbacks de botão ---

void setBBQTempPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    if (!setBBQTemp.getValue(&value)) {
        logHandler.logMessage("Nextion: falha ao ler setBBQTemp");
        return;
    }

    // D-02: validação de range
    int temp = static_cast<int>(value);
    if (temp < 30 || temp > 250) {
        logHandler.logMessage("Nextion: BBQ temp fora do range: " + String(temp));
        return;
    }

    sysStatLock();
    systemStatus->bbqTemperature = temp;
    sysStatUnlock();

    if (!FileSystem::saveConfigToFile(*systemStatus))
        logHandler.logError("Nextion: falha ao persistir BBQ temp");

    logHandler.logMessage("BBQ temp setada: " + String(temp));
    monitor.show();
}

void setChunkTempPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    if (!setChunkTemp.getValue(&value)) {
        logHandler.logMessage("Nextion: falha ao ler setChunkTemp");
        return;
    }

    // D-02: validação de range
    int temp = static_cast<int>(value);
    if (temp < 25 || temp > 100) {
        logHandler.logMessage("Nextion: proteína temp fora do range: " + String(temp));
        return;
    }

    sysStatLock();
    systemStatus->proteinTemperature = temp;
    sysStatUnlock();

    if (!FileSystem::saveConfigToFile(*systemStatus))
        logHandler.logError("Nextion: falha ao persistir proteína temp");

    logHandler.logMessage("Proteína temp setada: " + String(temp));
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);
    resetSystem(*systemStatus);
    // Salva estado limpo após reset (setpoints zerados, calibração preservada)
    if (!FileSystem::saveConfigToFile(*systemStatus))
        logHandler.logError("Nextion: falha ao persistir após reset");
}

void setCaliPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t bbq, chunk;
    if (!caliBBQTemp.getValue(&bbq)) {
        logHandler.logMessage("Nextion: falha ao ler caliBBQTemp");
        return;
    }
    if (!caliChunkTemp.getValue(&chunk)) {
        logHandler.logMessage("Nextion: falha ao ler caliChunkTemp");
        return;
    }

    sysStatLock();
    systemStatus->tempCalibration  = static_cast<int>(bbq);
    systemStatus->tempCalibrationP = static_cast<int>(chunk);
    sysStatUnlock();

    if (!FileSystem::saveConfigToFile(*systemStatus))
        logHandler.logError("Nextion: falha ao persistir calibração");

    logHandler.logMessage("Calibração: BBQ=" + String(bbq) + " Chunk=" + String(chunk));
    menu.show();
}

// --- Inicialização ---

void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17, false, 256);
    nexInit();

    setBBQTempPush.attachPush(setBBQTempPushCallback,     &sysStat);
    setChunkTempPush.attachPush(setChunkTempPushCallback, &sysStat);
    stopPush.attachPush(setStopPushCallback,              &sysStat);
    setCaliPush.attachPush(setCaliPushCallback,           &sysStat);

    nexCache = {};
    delay(100);
}

// --- Comunicação serial ---

uint8_t getCurrentPageId()
{
    // D-01: flush de bytes residuais do nexLoop antes de enviar "sendme"
    while (nexSerial.available()) nexSerial.read();

    nexSerial.print("sendme");
    nexSerial.write(0xff);
    nexSerial.write(0xff);
    nexSerial.write(0xff);

    // B-02: 25ms é suficiente para 5 bytes a 9600 baud (~5ms de transmissão)
    delay(25);

    if (nexSerial.available() >= 5 && nexSerial.read() == 0x66) {
        uint8_t pageId = nexSerial.read();
        nexSerial.read(); nexSerial.read(); nexSerial.read(); // consume 0xFF x3
        return pageId;
    }

    return NEXTION_PAGE_NONE;
}

void setPageBackground(const char *pageName, uint32_t img_id)
{
    static char cmdBuffer[50];
    snprintf(cmdBuffer, sizeof(cmdBuffer), "%s.pic=%d", pageName, img_id);

    while (nexSerial.available()) nexSerial.read();

    nexSerial.print(cmdBuffer);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
}

// --- Funções de update do display ---
// B-03: cada função copia os campos necessários do sysStat sob mutex,
//       libera o mutex, e depois faz as escritas seriais fora do lock.

static void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, bool forceUpdate)
{
    if (lastValue != newValue || forceUpdate) {
        component.setValue(static_cast<int32_t>(newValue));
        lastValue = newValue;
    }
}

void updateNextionMonitorVariables(SystemStatus &sysStat, uint8_t pageId)
{
    const unsigned long UPDATE_INTERVAL = 500;
    unsigned long currentTime = millis();
    if (currentTime - nexCache.lastUpdate < UPDATE_INTERVAL) return;
    nexCache.lastUpdate = currentTime;

    if (pageId != NEXTION_PAGE_MONITOR) {
        nexCache.lastPageId = pageId;
        return;
    }

    // B-03: copia sob mutex
    int currBBQTemp, currProbeTemp, currBBQTarget, currProbeTarget, currAvgTemp;
    bool currRelayOn;
    sysStatLock();
    currBBQTemp     = sysStat.calibratedTemp;
    currProbeTemp   = sysStat.calibratedTempP;
    currBBQTarget   = sysStat.bbqTemperature;
    currProbeTarget = sysStat.proteinTemperature;
    currAvgTemp     = sysStat.averageTemp;
    currRelayOn     = sysStat.isRelayOn;
    sysStatUnlock();

    // Escritas seriais fora do mutex
    if (currBBQTemp != nexCache.lastBBQTemp) {
        bbqTemp.setValue(currBBQTemp);
        nexCache.lastBBQTemp = currBBQTemp;
    }
    if (currProbeTemp != nexCache.lastProbeTemp) {
        chunkTemp.setValue(currProbeTemp);
        nexCache.lastProbeTemp = currProbeTemp;
    }
    if (currBBQTarget != nexCache.lastBBQTarget) {
        bbqTempSet.setValue(currBBQTarget);
        nexCache.lastBBQTarget = currBBQTarget;
    }
    if (currProbeTarget != nexCache.lastProbeTarget) {
        chunkTempSet.setValue(currProbeTarget);
        nexCache.lastProbeTarget = currProbeTarget;
    }
    if (currAvgTemp != nexCache.lastAvgTemp) {
        bbqTempAvg.setValue(currAvgTemp);
        nexCache.lastAvgTemp = currAvgTemp;
    }
    if (currRelayOn != nexCache.lastRelayState) {
        setPageBackground("monitor", currRelayOn ? NEXTION_BG_RELAY_ON : NEXTION_BG_RELAY_OFF);
        nexCache.lastRelayState = currRelayOn;
    }
}

void updateNextionSetBBQVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdBBQ);

    if (pageId != NEXTION_PAGE_BBQ_TEMP) {
        lastPageIdBBQ = pageId;
        initialUpdateDoneBBQ = false;
        return;
    }

    // B-03: copia sob mutex
    int currBBQTemp, currMin, currMax;
    sysStatLock();
    currBBQTemp = sysStat.bbqTemperature;
    currMin     = sysStat.minBBQTemp;
    currMax     = sysStat.maxBBQTemp;
    sysStatUnlock();

    if (!initialUpdateDoneBBQ) {
        int value = currBBQTemp > 0 ? currBBQTemp : currMin;
        setBBQTemp.setValue(value);
        initialUpdateDoneBBQ = true;
    }

    updateNumberComponent(minBBQTemp, lastMinBBQTemp, currMin, forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, currMax, forceUpdate);
    lastPageIdBBQ = pageId;
}

void updateNextionSetChunkVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdChunk);

    if (pageId != NEXTION_PAGE_CHUNK_TEMP) {
        lastPageIdChunk = pageId;
        initialUpdateDoneChunk = false;
        return;
    }

    // B-03: copia sob mutex
    int currProbeTemp, currMin, currMax;
    sysStatLock();
    currProbeTemp = sysStat.proteinTemperature;
    currMin       = sysStat.minPrtTemp;
    currMax       = sysStat.maxPrtTemp;
    sysStatUnlock();

    if (!initialUpdateDoneChunk) {
        int value = currProbeTemp > 0 ? currProbeTemp : currMin;
        setChunkTemp.setValue(value);
        initialUpdateDoneChunk = true;
    }

    updateNumberComponent(minChunkTemp, lastMinChunkTemp, currMin, forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, currMax, forceUpdate);
    lastPageIdChunk = pageId;
}

void updateNextionSetCaliVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdCali);

    if (pageId != NEXTION_PAGE_CALIBRATION) {
        lastPageIdCali = pageId;
        initialUpdateDoneCali = false;
        return;
    }

    // B-03: copia sob mutex
    int currCaliBBQ, currCaliChunk, currMinBBQ, currMaxBBQ, currMinChunk, currMaxChunk;
    sysStatLock();
    currCaliBBQ   = sysStat.tempCalibration;
    currCaliChunk = sysStat.tempCalibrationP;
    currMinBBQ    = sysStat.minCaliTemp;
    currMaxBBQ    = sysStat.maxCaliTemp;
    currMinChunk  = sysStat.minCaliTempP;
    currMaxChunk  = sysStat.maxCaliTempP;
    sysStatUnlock();

    if (!initialUpdateDoneCali) {
        caliBBQTemp.setValue(static_cast<int32_t>(currCaliBBQ));
        caliChunkTemp.setValue(static_cast<int32_t>(currCaliChunk));
        initialUpdateDoneCali = true;
    }

    updateNumberComponent(minCaliBBQTemp, lastMinCaliBBQ,   static_cast<float>(currMinBBQ),   forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, lastMaxCaliBBQ,   static_cast<float>(currMaxBBQ),   forceUpdate);
    updateNumberComponent(minCaliChuTemp, lastMinCaliChunk, static_cast<float>(currMinChunk), forceUpdate);
    updateNumberComponent(maxCaliChuTemp, lastMaxCaliChunk, static_cast<float>(currMaxChunk), forceUpdate);
    lastPageIdCali = pageId;
}
