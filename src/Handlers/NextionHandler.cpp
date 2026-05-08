#include "Handlers/NextionHandler.h"
#include "Handlers/NextionComponents.h"
#include "Handlers/LogHandler.h"
#include "SysStatMutex.h"

extern LogHandler logHandler;

#define nexSerial Serial2

// State tracking for change detection
static float lastMinBBQTemp = 0;
static float lastMaxBBQTemp = 0;
static float lastMinChunkTemp = 0;
static float lastMaxChunkTemp = 0;
static float lastMinCaliBBQ = 0;
static float lastMaxCaliBBQ = 0;
static float lastMinCaliChunk = 0;
static float lastMaxCaliChunk = 0;

static uint32_t lastPageIdBBQ   = -1;
static uint32_t lastPageIdChunk = -1;
static uint32_t lastPageIdCali  = -1;

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
    uint32_t lastPageId = 0;
    unsigned long lastUpdate = 0;
} nexCache;

void setBBQTempPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    if (!setBBQTemp.getValue(&value)) {
        logHandler.logMessage("Error: Failed to get value from setBBQTemp");
        return;
    }

    sysStatLock();
    systemStatus->bbqTemperature = static_cast<int>(value);
    sysStatUnlock();

    logHandler.logMessage("BBQTempValue: " + String(value));
    monitor.show();
}

void setChunkTempPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    if (!setChunkTemp.getValue(&value)) {
        logHandler.logMessage("Error: Failed to get value from setChunkTemp");
        return;
    }

    sysStatLock();
    systemStatus->proteinTemperature = static_cast<int>(value);
    sysStatUnlock();

    logHandler.logMessage("ChunkTempValue: " + String(value));
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);
    resetSystem(*systemStatus);
}

void setCaliPushCallback(void *ptr)
{
    if (ptr == nullptr) return;
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t bbq, chunk;
    if (!caliBBQTemp.getValue(&bbq)) {
        logHandler.logMessage("Error: Failed to get value from Cali BBQ");
        return;
    }
    if (!caliChunkTemp.getValue(&chunk)) {
        logHandler.logMessage("Error: Failed to get value from Cali Chunk");
        return;
    }

    sysStatLock();
    systemStatus->tempCalibration  = static_cast<int>(bbq);
    systemStatus->tempCalibrationP = static_cast<int>(chunk);
    sysStatUnlock();

    logHandler.logMessage("CaliBBQValue: " + String(bbq) + " CaliChunkValue: " + String(chunk));
    menu.show();
}

void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17, false, 256);
    nexInit();

    setBBQTempPush.attachPush(setBBQTempPushCallback,   &sysStat);
    setChunkTempPush.attachPush(setChunkTempPushCallback, &sysStat);
    stopPush.attachPush(setStopPushCallback,            &sysStat);
    setCaliPush.attachPush(setCaliPushCallback,         &sysStat);

    nexCache = {};
    delay(100);
}

uint8_t getCurrentPageId()
{
    uint8_t pageId = 0xFF;
    nexSerial.print("sendme");
    nexSerial.write(0xff);
    nexSerial.write(0xff);
    nexSerial.write(0xff);

    delay(100);

    if (nexSerial.available() >= 5 && nexSerial.read() == 0x66) {
        pageId = nexSerial.read();
        nexSerial.read();
        nexSerial.read();
        nexSerial.read();
    }

    return pageId;
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
    if (sysStat.isRelayOn != nexCache.lastRelayState) {
        setPageBackground("monitor", sysStat.isRelayOn ? 4 : 1);
        nexCache.lastRelayState = sysStat.isRelayOn;
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

    if (!initialUpdateDoneBBQ) {
        int value = sysStat.bbqTemperature > 0 ? sysStat.bbqTemperature : sysStat.minBBQTemp;
        setBBQTemp.setValue(value);
        initialUpdateDoneBBQ = true;
    }

    updateNumberComponent(minBBQTemp, lastMinBBQTemp, sysStat.minBBQTemp, forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, sysStat.maxBBQTemp, forceUpdate);
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

    if (!initialUpdateDoneChunk) {
        int value = sysStat.proteinTemperature > 0 ? sysStat.proteinTemperature : sysStat.minPrtTemp;
        setChunkTemp.setValue(value);
        initialUpdateDoneChunk = true;
    }

    updateNumberComponent(minChunkTemp, lastMinChunkTemp, sysStat.minPrtTemp,  forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, sysStat.maxPrtTemp,  forceUpdate);
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

    if (!initialUpdateDoneCali) {
        caliBBQTemp.setValue(static_cast<int32_t>(sysStat.tempCalibration));
        caliChunkTemp.setValue(static_cast<int32_t>(sysStat.tempCalibrationP));
        initialUpdateDoneCali = true;
    }

    updateNumberComponent(minCaliBBQTemp, lastMinCaliBBQ,   static_cast<float>(sysStat.minCaliTemp),  forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, lastMaxCaliBBQ,   sysStat.maxCaliTemp,                       forceUpdate);
    updateNumberComponent(minCaliChuTemp, lastMinCaliChunk, static_cast<float>(sysStat.minCaliTempP), forceUpdate);
    updateNumberComponent(maxCaliChuTemp, lastMaxCaliChunk, sysStat.maxCaliTempP,                      forceUpdate);
    lastPageIdCali = pageId;
}
