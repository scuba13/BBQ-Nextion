#include "NextionHandler.h"
#include "LogHandler.h"

extern LogHandler _logger; // Certifique-se de que o logHandler esteja declarado externamente ou passado como argumento

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


// Definition of Nextion components
NexPage wifi = NexPage(0, 0, "wifi");
NexPage welcome = NexPage(1, 0, "welcome");
NexPage menu = NexPage(2, 0, "menu");
NexPage monitor = NexPage(3, 0, "monitor");
NexPage BBQTemp = NexPage(4, 0, "BBQTemp");
NexPage ChunkTemp = NexPage(5, 0, "ChunkTemp");
NexPage energyPg = NexPage(6, 0, "energyPg");

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
    _logger.logMessage("Entering setBBQTempPopCallback");

    if (ptr == nullptr)
    {
        _logger.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    bool success = setBBQTemp.getValue(&value);

    if (!success)
    {
        _logger.logMessage("Error: Failed to get value from setBBQTemp");
        return;
    }

    int bbqTempValue = static_cast<int>(value);
    systemStatus->bbqTemperature = bbqTempValue;

    _logger.logMessage("BBQTempValue: " + String(bbqTempValue));
    _logger.logMessage("SystemStatus BBQ Temperature: " + String(systemStatus->bbqTemperature));
    _logger.logMessage("Exiting setBBQTempPopCallback");
    monitor.show();
}

void setChunkTempPushCallback(void *ptr)
{
    _logger.logMessage("Entering setChunkTempPopCallback");

    if (ptr == nullptr)
    {
        _logger.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    bool success = setChunkTemp.getValue(&value);

    if (!success)
    {
        _logger.logMessage("Error: Failed to get value from setChunkTemp");
        return;
    }

    int chunkTempValue = static_cast<int>(value);
    systemStatus->proteinTemperature = chunkTempValue;

    _logger.logMessage("ChunkTempValue: " + String(chunkTempValue));
    _logger.logMessage("SystemStatus Chunk Temperature: " + String(systemStatus->proteinTemperature));
    _logger.logMessage("Exiting setChunkTempPopCallback");
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    _logger.logMessage("Entering setStopPushCallback");

    if (ptr == nullptr)
    {
        _logger.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    resetSystem(*systemStatus);

    _logger.logMessage("Exiting setStopPushCallback");
}

void setCaliPushCallback(void *ptr)
{
    _logger.logMessage("Entering setCaliPushCallback");

    if (ptr == nullptr)
    {
        _logger.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t bbq;
    bool successBBQ = caliBBQTemp.getValue(&bbq);

    if (!successBBQ)
    {
        _logger.logMessage("Error: Failed to get value from Cali BBQ");
        return;
    }

    uint32_t chunk;
    bool successChunk = caliChunkTemp.getValue(&chunk);

    if (!successChunk)
    {
        _logger.logMessage("Error: Failed to get value from Cali Chunk");
        return;
    }

    int caliBBQValue = static_cast<int>(bbq);
    systemStatus->tempCalibration = caliBBQValue;

    int caliChunkValue = static_cast<int>(chunk);
    systemStatus->tempCalibrationP = caliChunkValue;

    _logger.logMessage("CaliBBQValue: " + String(caliBBQValue));
    _logger.logMessage("SystemStatus tempCalibration: " + String(systemStatus->tempCalibration));
    _logger.logMessage("CaliChunkValue: " + String(caliChunkValue));
    _logger.logMessage("SystemStatus tempCalibrationP: " + String(systemStatus->tempCalibrationP));
    _logger.logMessage("Exiting setCaliPushCallback");
    menu.show();
}

void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // Configura a porta serial para o Nextion (pinos RX2 e TX2)
    nexInit();
    setBBQTempPush.attachPush(setBBQTempPushCallback, &sysStat);
    setChunkTempPush.attachPush(setChunkTempPushCallback, &sysStat);
    stopPush.attachPush(setStopPushCallback, &sysStat);
    setCaliPush.attachPush(setCaliPushCallback, &sysStat);

    _logger.logMessage("Nextion initialized");

    delay(500);
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

    _logger.logMessage("Current Page ID: " + String(pageId));

    return pageId;
}

void setPageBackground(const char *pageName, uint32_t img_id)
{
    while (nexSerial.available())
    {
        nexSerial.read();
    }

    String cmd = String(pageName) + ".pic=" + String(img_id);
    _logger.logMessage("Command: " + cmd);
    nexSerial.print(cmd);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    delay(50);
}

void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, const char *componentName, bool forceUpdate)
{
    if (lastValue != newValue || forceUpdate)
    {
        component.setValue(static_cast<int32_t>(newValue)); // Use int32_t to support negative values
        lastValue = newValue;
        _logger.logMessage(String(componentName) + " updated to: " + String(newValue));
    }
}

void updateNextionMonitorVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageId);

    if (currentPageId != 3)
    {
        lastPageId = currentPageId;
        return;
    }

    _logger.logMessage("Updating Nextion Monitor Variables...");

    updateNumberComponent(bbqTempSet, lastBbqTempSet, sysStat.bbqTemperature, "bbqTempSet", forceUpdate);
    updateNumberComponent(bbqTemp, lastBbqTemp, sysStat.calibratedTemp, "bbqTemp", forceUpdate);
    updateNumberComponent(chunkTempSet, lastChunkTempSet, sysStat.proteinTemperature, "chunkTempSet", forceUpdate);
    updateNumberComponent(chunkTemp, lastChunkTemp, sysStat.calibratedTempP, "chunkTemp", forceUpdate);
    updateNumberComponent(bbqTempAvg, lastBbqTempAvg, sysStat.averageTemp, "bbqTempAvg", forceUpdate);

    if (sysStat.isRelayOn != lastRelayState || forceUpdate)
    {
        _logger.logMessage("Updating relay state to: " + String(sysStat.isRelayOn ? "ON" : "OFF"));
        if (sysStat.isRelayOn)
        {
            setPageBackground("monitor", 4);
        }
        else
        {
            setPageBackground("monitor", 1);
        }
        lastRelayState = sysStat.isRelayOn;
    }

    lastPageId = currentPageId;
}

void updateNextionSetBBQVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdBBQ);

    if (currentPageId != 4)
    {
        lastPageIdBBQ = currentPageId;
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
        _logger.logMessage("BBQTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minBBQTemp, lastMinBBQTemp, sysStat.minBBQTemp, "minBBQTemp", forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, sysStat.maxBBQTemp, "maxBBQTemp", forceUpdate);

    lastPageIdBBQ = currentPageId;
}

void updateNextionSetChunkVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdChunk);

    if (currentPageId != 5)
    {
        lastPageIdChunk = currentPageId;
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
        _logger.logMessage("ChunkTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minChunkTemp, lastMinChunkTemp, sysStat.minPrtTemp, "minChunkTemp", forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, sysStat.maxPrtTemp, "maxChunkTemp", forceUpdate);

    lastPageIdChunk = currentPageId;
}

void updateNextionSetCaliVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdCali);

    if (currentPageId != 6)
    {
        lastPageIdCali = currentPageId;
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
        _logger.logMessage("Calibration page initialized with BBQ: " + String(bbq) + " Chunk: " + String(chunk));
    }

    updateNumberComponent(minCaliBBQTemp, lastMinCaliBBQ, static_cast<int32_t>(sysStat.minCaliTemp), "minCaliBBQTemp", forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, lastMaxCaliBBQ, sysStat.maxCaliTemp, "maxCaliBBQTemp", forceUpdate);
    updateNumberComponent(minCaliChuTemp, lastMinCaliChunk, static_cast<int32_t>(sysStat.minCaliTempP), "minCaliChuTemp", forceUpdate);
    updateNumberComponent(maxCaliChuTemp, lastMaxCaliChunk, sysStat.maxCaliTempP, "maxCaliChuTemp", forceUpdate);

    lastPageIdCali = currentPageId;
}
