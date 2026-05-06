#include "NextionHandler.h"
#include "LogHandler.h"
#include "SysStatMutex.h"

extern LogHandler logHandler; // Certifique-se de que o logHandler esteja declarado externamente ou passado como argumento

// Define nexSerial
#define nexSerial Serial2

// Store the last known values to check for changes
float lastBbqTempSet = 0;
float lastBbqTemp = 0;
float lastChunkTempSet = 0;
float lastChunkTemp = 0;
float lastBbqTempAvg = 0;
bool lastRelayState = false;
uint32_t lastPageId = -1;

static float lastMinBBQTemp = 0;
static float lastMaxBBQTemp = 0;
uint32_t lastPageIdBBQ = -1;
bool initialUpdateDoneBBQ = false; // Variable to check if the initial update is done

static float lastMinChunkTemp = 0;
static float lastMaxChunkTemp = 0;
uint32_t lastPageIdChunk = -1;
bool initialUpdateDoneChunk = false; // Variable to check if the initial update is done

static float lastMinCaliBBQ = 0;
static float lastMaxCaliBBQ = 0;
static float lastMinCaliChunk = 0;
static float lastMaxCaliChunk = 0;
uint32_t lastPageIdCali = -1;
bool initialUpdateDoneCali = false;

// Cache de valores para reduzir comunicação serial
static struct {
    int lastBBQTemp = -999;
    int lastProbeTemp = -999;
    int lastBBQTarget = -999;
    int lastProbeTarget = -999;
    int lastAvgTemp = -999;
    bool lastRelayState = false;
    uint32_t lastPageId = 0;
    unsigned long lastUpdate = 0;
} nexCache;

// Definition of Nextion components
NexPage wifi = NexPage(0, 0, "wifi");
NexPage welcome = NexPage(1, 0, "welcome");
NexPage menu = NexPage(2, 0, "menu");
NexPage monitor = NexPage(3, 0, "monitor");
NexPage BBQTemp = NexPage(4, 0, "BBQTemp");
NexPage ChunkTemp = NexPage(5, 0, "ChunkTemp");
NexPage ap = NexPage(7, 0, "ap");
NexPage initial = NexPage(8, 0, "init");

// Page Monitor
NexNumber bbqTempSet = NexNumber(3, 10, "bbqTempSet");
NexNumber bbqTemp = NexNumber(3, 1, "bbqTemp");
NexNumber chunkTempSet = NexNumber(3, 11, "chunkTempSet");
NexNumber chunkTemp = NexNumber(3, 2, "chunkTemp");
NexNumber bbqTempAvg = NexNumber(3, 8, "bbqTempAvg");
NexButton stopPush = NexButton(3, 18, "stop");

// Page BBQTemp
NexNumber setBBQTemp = NexNumber(4, 1, "setBBQTemp");
NexNumber minBBQTemp = NexNumber(4, 6, "minBBQTemp");
NexNumber maxBBQTemp = NexNumber(4, 7, "maxBBQTemp");
NexButton setBBQTempPush = NexButton(4, 8, "setBBQ");

// Page ChunkTemp
NexNumber setChunkTemp = NexNumber(5, 1, "setChunkTemp");
NexNumber minChunkTemp = NexNumber(5, 6, "minChunkTemp");
NexNumber maxChunkTemp = NexNumber(5, 7, "maxChunkTemp");
NexButton setChunkTempPush = NexButton(5, 8, "setChunk");


// Page Calibration
NexNumber caliBBQTemp = NexNumber(6, 4, "caliBBQTemp");
NexNumber minCaliBBQTemp = NexNumber(6, 12, "minCaliBBQTemp");
NexNumber maxCaliBBQTemp = NexNumber(6, 13, "maxCaliBBQTemp");

NexNumber caliChunkTemp = NexNumber(6, 7, "caliChunkTemp");
NexNumber minCaliChuTemp = NexNumber(6, 14, "minCaliChuTemp");
NexNumber maxCaliChuTemp = NexNumber(6, 15, "maxCaliChuTemp");

NexButton setCaliPush = NexButton(6, 10, "setCali");

NexTouch *nex_listen_list[] = {
    &setBBQTempPush,
    &setChunkTempPush,
    &stopPush,
    &setCaliPush,
    NULL};

char buffer[100] = {0};

void setBBQTempPushCallback(void *ptr)
{
    logHandler.logMessage("Entering setBBQTempPopCallback");

    if (ptr == nullptr)
    {
        logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    bool success = setBBQTemp.getValue(&value);

    if (!success)
    {
        logHandler.logMessage("Error: Failed to get value from setBBQTemp");
        return;
    }

    int bbqTempValue = static_cast<int>(value);
    sysStatLock();
    systemStatus->bbqTemperature = bbqTempValue;
    sysStatUnlock();

    logHandler.logMessage("BBQTempValue: " + String(bbqTempValue));
    logHandler.logMessage("Exiting setBBQTempPopCallback");
    monitor.show();
}

void setChunkTempPushCallback(void *ptr)
{
    logHandler.logMessage("Entering setChunkTempPopCallback");

    if (ptr == nullptr)
    {
        logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    bool success = setChunkTemp.getValue(&value);

    if (!success)
    {
        logHandler.logMessage("Error: Failed to get value from setChunkTemp");
        return;
    }

    int chunkTempValue = static_cast<int>(value);
    sysStatLock();
    systemStatus->proteinTemperature = chunkTempValue;
    sysStatUnlock();

    logHandler.logMessage("ChunkTempValue: " + String(chunkTempValue));
    logHandler.logMessage("Exiting setChunkTempPopCallback");
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    logHandler.logMessage("Entering setStopPushCallback");

    if (ptr == nullptr)
    {
        logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    resetSystem(*systemStatus);

    logHandler.logMessage("Exiting setStopPushCallback");
}

void setCaliPushCallback(void *ptr)
{
    logHandler.logMessage("Entering setCaliPushCallback");

    if (ptr == nullptr)
    {
        logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t bbq;
    bool successBBQ = caliBBQTemp.getValue(&bbq);

    if (!successBBQ)
    {
        logHandler.logMessage("Error: Failed to get value from Cali BBQ");
        return;
    }

    uint32_t chunk;
    bool successChunk = caliChunkTemp.getValue(&chunk);

    if (!successChunk)
    {
        logHandler.logMessage("Error: Failed to get value from Cali Chunk");
        return;
    }

    int caliBBQValue = static_cast<int>(bbq);
    int caliChunkValue = static_cast<int>(chunk);
    sysStatLock();
    systemStatus->tempCalibration = caliBBQValue;
    systemStatus->tempCalibrationP = caliChunkValue;
    sysStatUnlock();

    logHandler.logMessage("CaliBBQValue: " + String(caliBBQValue));
    logHandler.logMessage("CaliChunkValue: " + String(caliChunkValue));
    logHandler.logMessage("Exiting setCaliPushCallback");
    menu.show();
}

void initNextion(SystemStatus &sysStat)
{
    // Configura serial com buffer maior
    Serial2.begin(9600, SERIAL_8N1, 16, 17, false, 256);
    nexInit();

    // Registra callbacks
    setBBQTempPush.attachPush(setBBQTempPushCallback, &sysStat);
    setChunkTempPush.attachPush(setChunkTempPushCallback, &sysStat);
    stopPush.attachPush(setStopPushCallback, &sysStat);
    setCaliPush.attachPush(setCaliPushCallback, &sysStat);

    // Reset do cache
    nexCache = {};
    
    delay(100); // Pequeno delay para estabilização
}

uint8_t getCurrentPageId()
{
    uint8_t pageId = 0xFF; // Default to invalid page ID
    String cmd = "sendme";
    nexSerial.print(cmd);
    nexSerial.write(0xff);
    nexSerial.write(0xff);
    nexSerial.write(0xff);

    delay(100);

    if (nexSerial.available() >= 5)
    {
        if (nexSerial.read() == 0x66)
        {
            pageId = nexSerial.read();
            nexSerial.read(); // Consume 0xff
            nexSerial.read(); // Consume 0xff
            nexSerial.read(); // Consume 0xff
        }
    }

    //logHandler.logMessage("Current Page ID: " + String(pageId));

    return pageId;
}

void setPageBackground(const char *pageName, uint32_t img_id)
{
    static char cmdBuffer[50];
    snprintf(cmdBuffer, sizeof(cmdBuffer), "%s.pic=%d", pageName, img_id);
    
    // Limpa buffer serial
    while (nexSerial.available()) {
        nexSerial.read();
    }
    
    // Envia comando
    nexSerial.print(cmdBuffer);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
}

void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, const char *componentName, bool forceUpdate)
{
    if (lastValue != newValue || forceUpdate)
    {
        component.setValue(static_cast<int32_t>(newValue)); // Use int32_t to support negative values
        lastValue = newValue;
       // logHandler.logMessage(String(componentName) + " updated to: " + String(newValue));
    }
}

void updateNextionMonitorVariables(SystemStatus &sysStat, uint8_t pageId)
{
    const unsigned long UPDATE_INTERVAL = 500;
    unsigned long currentTime = millis();

    if (currentTime - nexCache.lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    nexCache.lastUpdate = currentTime;

    if (pageId != 3) {
        nexCache.lastPageId = pageId;
        return;
    }

    // Atualiza apenas valores que mudaram
    if (sysStat.calibratedTemp != nexCache.lastBBQTemp) {
        bbqTemp.setValue(sysStat.calibratedTemp);
        nexCache.lastBBQTemp = sysStat.calibratedTemp;
    }

    if (sysStat.calibratedTempP != nexCache.lastProbeTemp) {
        chunkTemp.setValue(sysStat.calibratedTempP);
        nexCache.lastProbeTemp = sysStat.calibratedTempP;
    }

    if (sysStat.bbqTemperature != nexCache.lastBBQTarget) {
        bbqTempSet.setValue(sysStat.bbqTemperature);
        nexCache.lastBBQTarget = sysStat.bbqTemperature;
    }

    if (sysStat.proteinTemperature != nexCache.lastProbeTarget) {
        chunkTempSet.setValue(sysStat.proteinTemperature);
        nexCache.lastProbeTarget = sysStat.proteinTemperature;
    }

    if (sysStat.averageTemp != nexCache.lastAvgTemp) {
        bbqTempAvg.setValue(sysStat.averageTemp);
        nexCache.lastAvgTemp = sysStat.averageTemp;
    }

    // Atualiza o fundo apenas se o estado do relé mudou
    if (sysStat.isRelayOn != nexCache.lastRelayState) {
        setPageBackground("monitor", sysStat.isRelayOn ? 4 : 1);
        nexCache.lastRelayState = sysStat.isRelayOn;
    }
}

void updateNextionSetBBQVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdBBQ);

    if (pageId != 4)
    {
        lastPageIdBBQ = pageId;
        initialUpdateDoneBBQ = false;
        return;
    }

    if (!initialUpdateDoneBBQ)
    {
        int value = sysStat.bbqTemperature;

        if (value == 0)
        {
            value = sysStat.minBBQTemp;
        }

        setBBQTemp.setValue(value);
        initialUpdateDoneBBQ = true;
       // logHandler.logMessage("BBQTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minBBQTemp, lastMinBBQTemp, sysStat.minBBQTemp, "minBBQTemp", forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, sysStat.maxBBQTemp, "maxBBQTemp", forceUpdate);

    lastPageIdBBQ = pageId;
}

void updateNextionSetChunkVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdChunk);

    if (pageId != 5)
    {
        lastPageIdChunk = pageId;
        initialUpdateDoneChunk = false;
        return;
    }

    if (!initialUpdateDoneChunk)
    {
        int value = sysStat.proteinTemperature;

        if (value == 0)
        {
            value = sysStat.minPrtTemp;
        }

        setChunkTemp.setValue(value);
        initialUpdateDoneChunk = true;
       // logHandler.logMessage("ChunkTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minChunkTemp, lastMinChunkTemp, sysStat.minPrtTemp, "minChunkTemp", forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, sysStat.maxPrtTemp, "maxChunkTemp", forceUpdate);

    lastPageIdChunk = pageId;
}

void updateNextionSetCaliVariables(SystemStatus &sysStat, uint8_t pageId)
{
    bool forceUpdate = (pageId != lastPageIdCali);

    if (pageId != 6)
    {
        lastPageIdCali = pageId;
        initialUpdateDoneCali = false;
        return;
    }

    if (!initialUpdateDoneCali)
    {
        int32_t bbq = static_cast<int32_t>(sysStat.tempCalibration);
        int32_t chunk = static_cast<int32_t>(sysStat.tempCalibrationP);

        caliBBQTemp.setValue(bbq);
        caliChunkTemp.setValue(chunk);

        initialUpdateDoneCali = true;
       // logHandler.logMessage("Calibration page initialized with BBQ: " + String(bbq) + " Chunk: " + String(chunk));
    }

    updateNumberComponent(minCaliBBQTemp, lastMinCaliBBQ, static_cast<int32_t>(sysStat.minCaliTemp), "minCaliBBQTemp", forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, lastMaxCaliBBQ, sysStat.maxCaliTemp, "maxCaliBBQTemp", forceUpdate);
    updateNumberComponent(minCaliChuTemp, lastMinCaliChunk, static_cast<int32_t>(sysStat.minCaliTempP), "minCaliChuTemp", forceUpdate);
    updateNumberComponent(maxCaliChuTemp, lastMaxCaliChunk, sysStat.maxCaliTempP, "maxCaliChuTemp", forceUpdate);

    lastPageIdCali = pageId;
}
