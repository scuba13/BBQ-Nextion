#include "NextionHandler.h"
#include "AppContext.h"

extern AppContext app; // Certifique-se de que o app esteja declarado externamente ou passado como argumento

// Define nexSerial
#define nexSerial Serial2

// Declaração antecipada de funções
void setPageBackground(const char *pageName, uint32_t img_id);
void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, bool forceUpdate);
uint32_t getCurrentPageId();

// Declaração dos componentes Nextion
// Pages
NexPage wifi = NexPage(0, 0, "wifi");
NexPage welcome = NexPage(1, 0, "welcome");
NexPage menu = NexPage(2, 0, "menu");
NexPage monitor = NexPage(3, 0, "monitor");
NexPage BBQTemp = NexPage(4, 0, "BBQTemp");
NexPage ChunkTemp = NexPage(5, 0, "ChunkTemp");
NexPage energyPg = NexPage(6, 0, "energyPg");
NexPage ap = NexPage(7, 0, "ap");
NexPage initial = NexPage(8, 0, "init");

// Monitor page components
NexNumber bbqTempSet = NexNumber(3, 10, "bbqTempSet");
NexNumber bbqTemp = NexNumber(3, 1, "bbqTemp");
NexNumber chunkTempSet = NexNumber(3, 11, "chunkTempSet");
NexNumber chunkTemp = NexNumber(3, 2, "chunkTemp");
NexNumber bbqTempAvg = NexNumber(3, 8, "bbqTempAvg");
NexButton stopPush = NexButton(3, 18, "stop");

// BBQ Temperature page components
NexNumber setBBQTemp = NexNumber(4, 1, "setBBQTemp");
NexNumber minBBQTemp = NexNumber(4, 6, "minBBQTemp");
NexNumber maxBBQTemp = NexNumber(4, 7, "maxBBQTemp");
NexButton setBBQTempPush = NexButton(4, 8, "setBBQ");

// Chunk Temperature page components
NexNumber setChunkTemp = NexNumber(5, 1, "setChunkTemp");
NexNumber minChunkTemp = NexNumber(5, 6, "minChunkTemp");
NexNumber maxChunkTemp = NexNumber(5, 7, "maxChunkTemp");
NexButton setChunkTempPush = NexButton(5, 8, "setChunk");

// Calibration page components
NexNumber caliBBQTemp = NexNumber(6, 4, "caliBBQTemp");
NexNumber minCaliBBQTemp = NexNumber(6, 12, "minCaliBBQTemp");
NexNumber maxCaliBBQTemp = NexNumber(6, 13, "maxCaliBBQTemp");
NexNumber caliChunkTemp = NexNumber(6, 7, "caliChunkTemp");
NexNumber minCaliChuTemp = NexNumber(6, 14, "minCaliChuTemp");
NexNumber maxCaliChuTemp = NexNumber(6, 15, "maxCaliChuTemp");
NexButton setCaliPush = NexButton(6, 10, "setCali");

// Lista de componentes touch
NexTouch *nex_listen_list[] = {
    &setBBQTempPush,
    &setChunkTempPush,
    &stopPush,
    &setCaliPush,
    NULL
};

// Variáveis de estado para as páginas
static struct {
    float lastMinBBQTemp = 0;
    float lastMaxBBQTemp = 0;
    uint32_t lastPageIdBBQ = -1;
    bool initialUpdateDoneBBQ = false;

    float lastMinChunkTemp = 0;
    float lastMaxChunkTemp = 0;
    uint32_t lastPageIdChunk = -1;
    bool initialUpdateDoneChunk = false;

    float lastMinCaliBBQ = 0;
    float lastMaxCaliBBQ = 0;
    float lastMinCaliChunk = 0;
    float lastMaxCaliChunk = 0;
    uint32_t lastPageIdCali = -1;
    bool initialUpdateDoneCali = false;
} pageState;

// Cache de valores para reduzir atualizações desnecessárias
static struct {
    float bbqTempSet = 0;
    float bbqTemp = 0;
    float chunkTempSet = 0;
    float chunkTemp = 0;
    float bbqTempAvg = 0;
    bool relayState = false;
    uint32_t pageId = -1;
    unsigned long lastUpdate = 0;
} displayCache;

// Constantes para otimização
constexpr unsigned long UPDATE_INTERVAL = 250;  // 4 atualizações por segundo
constexpr float TEMP_THRESHOLD = 0.5;  // Limiar para atualização de temperatura

// Função auxiliar para atualizar componente numérico com threshold
inline bool shouldUpdateNumber(float lastValue, float newValue) {
    return abs(lastValue - newValue) >= TEMP_THRESHOLD;
}

// Otimizar atualização de componentes numéricos
void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, bool forceUpdate) {
    if (forceUpdate || shouldUpdateNumber(lastValue, newValue)) {
        component.setValue(static_cast<int32_t>(newValue));
        lastValue = newValue;
    }
}

// Otimizar verificação de página atual
uint32_t getCurrentPageId() {
    static unsigned long lastCheck = 0;
    static uint32_t lastPageId = -1;
    
    unsigned long now = millis();
    if (now - lastCheck < 100) {  // Limitar verificações a cada 100ms
        return lastPageId;
    }
    
    lastCheck = now;
    nexSerial.print("sendme");
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    
    if (nexSerial.available()) {
        lastPageId = nexSerial.read();
        // Limpar buffer
        while (nexSerial.available()) {
            nexSerial.read();
        }
    }
    
    return lastPageId;
}

void updateNextionMonitorVariables(SystemStatus &sysStat) {
    unsigned long now = millis();
    if (now - displayCache.lastUpdate < UPDATE_INTERVAL) {
        return;  // Limitar taxa de atualização
    }
    displayCache.lastUpdate = now;
    
    uint32_t currentPageId = getCurrentPageId();
    if (currentPageId != 3) {  // Página do monitor
        displayCache.pageId = currentPageId;
        return;
    }
    
    bool forceUpdate = (currentPageId != displayCache.pageId);
    
    // Atualizar componentes apenas se necessário
    updateNumberComponent(bbqTempSet, displayCache.bbqTempSet, sysStat.bbqTemperature, forceUpdate);
    updateNumberComponent(bbqTemp, displayCache.bbqTemp, sysStat.calibratedTemp, forceUpdate);
    updateNumberComponent(chunkTempSet, displayCache.chunkTempSet, sysStat.proteinTemperature, forceUpdate);
    updateNumberComponent(chunkTemp, displayCache.chunkTemp, sysStat.calibratedTempP, forceUpdate);
    updateNumberComponent(bbqTempAvg, displayCache.bbqTempAvg, sysStat.averageTemp, forceUpdate);
    
    // Atualizar estado do relé apenas se mudou
    if (forceUpdate || sysStat.isRelayOn != displayCache.relayState) {
        setPageBackground("monitor", sysStat.isRelayOn ? 4 : 1);
        displayCache.relayState = sysStat.isRelayOn;
    }
    
    displayCache.pageId = currentPageId;
}

// Otimizar callbacks
void setBBQTempPushCallback(void *ptr) {
    uint32_t value;
    if (setBBQTemp.getValue(&value)) {
        app.sysStat.bbqTemperature = static_cast<int>(value);
        monitor.show();
    }
}

void setChunkTempPushCallback(void *ptr)
{
    app.logHandler.logMessage("Entering setChunkTempPopCallback");

    if (ptr == nullptr)
    {
        app.logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t value;
    bool success = setChunkTemp.getValue(&value);

    if (!success)
    {
        app.logHandler.logMessage("Error: Failed to get value from setChunkTemp");
        return;
    }

    int chunkTempValue = static_cast<int>(value);
    systemStatus->proteinTemperature = chunkTempValue;

    app.logHandler.logMessage("ChunkTempValue: " + String(chunkTempValue));
    app.logHandler.logMessage("SystemStatus Chunk Temperature: " + String(systemStatus->proteinTemperature));
    app.logHandler.logMessage("Exiting setChunkTempPopCallback");
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    app.logHandler.logMessage("Entering setStopPushCallback");

    if (ptr == nullptr)
    {
        app.logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    resetSystem(*systemStatus);

    app.logHandler.logMessage("Exiting setStopPushCallback");
}

void setCaliPushCallback(void *ptr)
{
    app.logHandler.logMessage("Entering setCaliPushCallback");

    if (ptr == nullptr)
    {
        app.logHandler.logMessage("Error: ptr is null");
        return;
    }

    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    uint32_t bbq;
    bool successBBQ = caliBBQTemp.getValue(&bbq);

    if (!successBBQ)
    {
        app.logHandler.logMessage("Error: Failed to get value from Cali BBQ");
        return;
    }

    uint32_t chunk;
    bool successChunk = caliChunkTemp.getValue(&chunk);

    if (!successChunk)
    {
        app.logHandler.logMessage("Error: Failed to get value from Cali Chunk");
        return;
    }

    int caliBBQValue = static_cast<int>(bbq);
    systemStatus->tempCalibration = caliBBQValue;

    int caliChunkValue = static_cast<int>(chunk);
    systemStatus->tempCalibrationP = caliChunkValue;

    app.logHandler.logMessage("CaliBBQValue: " + String(caliBBQValue));
    app.logHandler.logMessage("SystemStatus tempCalibration: " + String(systemStatus->tempCalibration));
    app.logHandler.logMessage("CaliChunkValue: " + String(caliChunkValue));
    app.logHandler.logMessage("SystemStatus tempCalibrationP: " + String(systemStatus->tempCalibrationP));
    app.logHandler.logMessage("Exiting setCaliPushCallback");
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

    app.logHandler.logMessage("Nextion initialized");

    delay(500);
}

void setPageBackground(const char *pageName, uint32_t img_id)
{
    while (nexSerial.available())
    {
        nexSerial.read();
    }

    String cmd = String(pageName) + ".pic=" + String(img_id);
   // _logger.logMessage("Command: " + cmd);
    nexSerial.print(cmd);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    delay(50);
}

void updateNextionSetBBQVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != pageState.lastPageIdBBQ);

    if (currentPageId != 4)
    {
        pageState.lastPageIdBBQ = currentPageId;
        pageState.initialUpdateDoneBBQ = false;
        return;
    }

    if (!pageState.initialUpdateDoneBBQ)
    {
        int value = sysStat.bbqTemperature;

        if (value == 0)
        {
            value = sysStat.minBBQTemp;
        }

        setBBQTemp.setValue(value);
        pageState.initialUpdateDoneBBQ = true;
       // _logger.logMessage("BBQTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minBBQTemp, pageState.lastMinBBQTemp, sysStat.minBBQTemp, forceUpdate);
    updateNumberComponent(maxBBQTemp, pageState.lastMaxBBQTemp, sysStat.maxBBQTemp, forceUpdate);

    pageState.lastPageIdBBQ = currentPageId;
}

void updateNextionSetChunkVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != pageState.lastPageIdChunk);

    if (currentPageId != 5)
    {
        pageState.lastPageIdChunk = currentPageId;
        pageState.initialUpdateDoneChunk = false;
        return;
    }

    if (!pageState.initialUpdateDoneChunk)
    {
        int value = sysStat.proteinTemperature;

        if (value == 0)
        {
            value = sysStat.minPrtTemp;
        }

        setChunkTemp.setValue(value);
        pageState.initialUpdateDoneChunk = true;
       // _logger.logMessage("ChunkTemp page initialized with value: " + String(value));
    }

    updateNumberComponent(minChunkTemp, pageState.lastMinChunkTemp, sysStat.minPrtTemp, forceUpdate);
    updateNumberComponent(maxChunkTemp, pageState.lastMaxChunkTemp, sysStat.maxPrtTemp, forceUpdate);

    pageState.lastPageIdChunk = currentPageId;
}

void updateNextionSetCaliVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != pageState.lastPageIdCali);

    if (currentPageId != 6)
    {
        pageState.lastPageIdCali = currentPageId;
        pageState.initialUpdateDoneCali = false;
        return;
    }

    if (!pageState.initialUpdateDoneCali)
    {
        int32_t bbq = static_cast<int32_t>(sysStat.tempCalibration);
        int32_t chunk = static_cast<int32_t>(sysStat.tempCalibrationP);

        caliBBQTemp.setValue(bbq);
        caliChunkTemp.setValue(chunk);

        pageState.initialUpdateDoneCali = true;
       // _logger.logMessage("Calibration page initialized with BBQ: " + String(bbq) + " Chunk: " + String(chunk));
    }

    updateNumberComponent(minCaliBBQTemp, pageState.lastMinCaliBBQ, static_cast<int32_t>(sysStat.minCaliTemp), forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, pageState.lastMaxCaliBBQ, sysStat.maxCaliTemp, forceUpdate);
    updateNumberComponent(minCaliChuTemp, pageState.lastMinCaliChunk, static_cast<int32_t>(sysStat.minCaliTempP), forceUpdate);
    updateNumberComponent(maxCaliChuTemp, pageState.lastMaxCaliChunk, sysStat.maxCaliTempP, forceUpdate);

    pageState.lastPageIdCali = currentPageId;
}
