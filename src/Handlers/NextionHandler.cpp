#include "NextionHandler.h"

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
    dbSerial.println("Entering setBBQTempPopCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        dbSerial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    // Get the value from setBBQTemp
    uint32_t value;
    bool success = setBBQTemp.getValue(&value);

    if (!success)
    {
        dbSerial.println("Error: Failed to get value from setBBQTemp");
        return;
    }

    // Cast value to int and set it to the bbqTemperature
    int bbqTempValue = static_cast<int>(value);
    systemStatus->bbqTemperature = bbqTempValue;

    // Debugging statements
    dbSerial.println("BBQTempValue: " + String(bbqTempValue));
    dbSerial.println("SystemStatus BBQ Temperature: " + String(systemStatus->bbqTemperature));
    dbSerial.println("Exiting setBBQTempPopCallback");
    monitor.show();
}

void setChunkTempPushCallback(void *ptr)
{
    dbSerial.println("Entering setChunkTempPopCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        dbSerial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    // Get the value from setBBQTemp
    uint32_t value;
    bool success = setChunkTemp.getValue(&value);

    if (!success)
    {
        dbSerial.println("Error: Failed to get value from setChunkTemp");
        return;
    }

    // Cast value to int and set it to the bbqTemperature
    int chunkTempValue = static_cast<int>(value);
    systemStatus->proteinTemperature = chunkTempValue;

    // Debugging statements
    dbSerial.println("BBQTempValue: " + String(chunkTempValue));
    dbSerial.println("SystemStatus Chunk Temperature: " + String(systemStatus->proteinTemperature));
    dbSerial.println("Exiting setChunkTempPopCallback");
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    dbSerial.println("Entering setStopPushCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        dbSerial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    resetSystem(*systemStatus);

    dbSerial.println("Exiting setStopPushCallback");
}

void setCaliPushCallback(void *ptr)
{
    dbSerial.println("Entering setCaliPushCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        dbSerial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    // Get the value from caliBBQTemp
    uint32_t bbq;
    bool successBBQ = caliBBQTemp.getValue(&bbq);

    if (!successBBQ)
    {
        dbSerial.println("Error: Failed to get value from Cali BBQ");
        return;
    }

    // Get the value from caliChunkTemp
    uint32_t chunk;
    bool successChunk = caliChunkTemp.getValue(&chunk);

    if (!successChunk)
    {
        dbSerial.println("Error: Failed to get value from Cali Chunk");
        return;
    }

    // Cast value to int and set it to the tempCalibration
    int caliBBQValue = static_cast<int>(bbq);
    systemStatus->tempCalibration = caliBBQValue;

        // Cast value to int and set it to the tempCalibrationP
    int caliChunkValue = static_cast<int>(chunk);
    systemStatus->tempCalibrationP = caliChunkValue;

    // Debugging statements
    dbSerial.println("CaliBBQValue: " + String(caliBBQValue));
    dbSerial.println("SystemStatus tempCalibration: " + String(systemStatus->tempCalibration));
    dbSerial.println("CaliChunkValue: " + String(caliChunkValue));
    dbSerial.println("SystemStatus tempCalibrationP: " + String(systemStatus->tempCalibrationP));
    dbSerial.println("Exiting setChunkTempPopCallback");
    monitor.show();
}


void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // Configura a porta serial para o Nextion (pinos RX2 e TX2)
    nexInit();
    setBBQTempPush.attachPush(setBBQTempPushCallback, &sysStat);
    setChunkTempPush.attachPush(setChunkTempPushCallback, &sysStat);
    stopPush.attachPush(setStopPushCallback, &sysStat);
    setCaliPush.attachPush(setCaliPushCallback, &sysStat);


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

    // Wait a little for the response
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

    // dbSerial.print("Current Page ID: ");
    // dbSerial.println(pageId);

    return pageId;
}

void setPageBackground(const char *pageName, uint32_t img_id)
{
    while (nexSerial.available())
    {
        nexSerial.read();
    }

    // Construct the command to change the background image
    String cmd = String(pageName) + ".pic=" + String(img_id);
    // dbSerial.println("Command: " + cmd);
    nexSerial.print(cmd);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    nexSerial.write(0xFF);
    delay(50); // Pequeno delay para garantir que o comando seja processado
}

// Function to update a Nextion number component if the value has changed or forceUpdate is true
void updateNumberComponent(NexNumber &component, float &lastValue, float newValue, const char *componentName, bool forceUpdate)
{
    if (lastValue != newValue || forceUpdate)
    {
        // dbSerial.print("Updating ");
        // dbSerial.print(componentName);
        // dbSerial.print(" from ");
        // dbSerial.print(lastValue);
        // dbSerial.print(" to ");
        // dbSerial.println(newValue);

        component.setValue(static_cast<uint32_t>(newValue));
        lastValue = newValue;
    }
    else
    {
        // dbSerial.print("No update needed for ");
        // dbSerial.print(componentName);
        // dbSerial.print(", value remains ");
        // dbSerial.println(lastValue);
    }
}

void updateNextionMonitorVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageId); // Force update if the page has changed

    // Check if the current page is the monitor page
    if (currentPageId != 3) // Assuming page ID 3 is the monitor page
    {
        lastPageId = currentPageId;
        return; // Exit if the current page is not the monitor page
    }

   // dbSerial.println("-------------------------");
    updateNumberComponent(bbqTempSet, lastBbqTempSet, sysStat.bbqTemperature, "bbqTempSet", forceUpdate);
    updateNumberComponent(bbqTemp, lastBbqTemp, sysStat.calibratedTemp, "bbqTemp", forceUpdate);
    updateNumberComponent(chunkTempSet, lastChunkTempSet, sysStat.proteinTemperature, "chunkTempSet", forceUpdate);
    updateNumberComponent(chunkTemp, lastChunkTemp, sysStat.calibratedTempP, "chunkTemp", forceUpdate);
    updateNumberComponent(bbqTempAvg, lastBbqTempAvg, sysStat.averageTemp, "bbqTempAvg", forceUpdate);

    if (sysStat.isRelayOn != lastRelayState || forceUpdate)
    {
        // dbSerial.print("Updating relay state from ");
        // dbSerial.print(lastRelayState);
        // dbSerial.print(" to ");
        // dbSerial.println(sysStat.isRelayOn);

        if (sysStat.isRelayOn)
        {
            // relayStatePic.setPic(0);
            setPageBackground("monitor", 4);
        }
        else
        {
            // relayStatePic.setPic(6);
            setPageBackground("monitor", 1);
        }
        lastRelayState = sysStat.isRelayOn;
    }
    else
    {
      //  dbSerial.print("No update needed for relay state, value remains ");
       // dbSerial.println(lastRelayState);
    }

   // dbSerial.println("-------------------------");

    // Update the last page ID
    lastPageId = currentPageId;
}

void updateNextionSetBBQVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdBBQ); // Force update if the page has changed

    // Check if the current page is the set BBQ page
    if (currentPageId != 4)
    {
        lastPageIdBBQ = currentPageId;
        initialUpdateDoneBBQ = false; // Reset the initial update flag when leaving the page
        return;
    }

    if (!initialUpdateDoneBBQ) // Perform the update only once when the page is loaded
    {
        int value = sysStat.bbqTemperature; 

        if (value == 0)
        {
            value = sysStat.minBBQTemp; 
        }

        setBBQTemp.setValue(value);    // Set the value on the Nextion component
        initialUpdateDoneBBQ = true; // Set the flag to true to prevent further updates
    }

   // dbSerial.println("-------------------------");
    updateNumberComponent(minBBQTemp, lastMinBBQTemp, sysStat.minBBQTemp, "minBBQTemp", forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, sysStat.maxBBQTemp, "maxBBQTemp", forceUpdate);
   // dbSerial.println("-------------------------");

    // Update the last page ID
    lastPageIdBBQ = currentPageId;
}

void updateNextionSetChunkVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdChunk); // Force update if the page has changed

    // Check if the current page is the set Chunk page
    if (currentPageId != 5)
    {
        lastPageIdChunk = currentPageId;
        initialUpdateDoneChunk = false; // Reset the initial update flag when leaving the page
        return;
    }

    if (!initialUpdateDoneChunk) // Perform the update only once when the page is loaded
    {
        int value = sysStat.proteinTemperature; // Get the value from sysStat.proteinTemperature

        if (value == 0)
        {
            value = sysStat.minPrtTemp; // Use sysStat.minPrtTemp if avgChunkTemp is zero
        }

        setChunkTemp.setValue(value);    // Set the value on the Nextion component
        
        initialUpdateDoneChunk = true; // Set the flag to true to prevent further updates
    }

    
    updateNumberComponent(minChunkTemp, lastMinChunkTemp, sysStat.minPrtTemp, "minChunkTemp", forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, sysStat.maxPrtTemp, "maxChunkTemp", forceUpdate);
   

    // Update the last page ID
    lastPageIdChunk = currentPageId;
}

void updateNextionSetCaliVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdCali); // Force update if the page has changed

    // Check if the current page is the set Chunk page
    if (currentPageId != 6)
    {
        lastPageIdCali = currentPageId;
        initialUpdateDoneCali = false; // Reset the initial update flag when leaving the page
        return;
    }

    if (!initialUpdateDoneCali) // Perform the update only once when the page is loaded
    {
        int bbq = sysStat.tempCalibration; // Get the value from sysStat.tempCalibration
        int chunk = sysStat.tempCalibrationP; // Get the value from sysStat.tempCalibrationP

        caliBBQTemp.setValue(bbq);    // Set the value on the Nextion component
        caliChunkTemp.setValue(chunk);    // Set the value on the Nextion component
        
        initialUpdateDoneChunk = true; // Set the flag to true to prevent further updates
    }

    updateNumberComponent(minCaliBBQTemp, lastMinCaliBBQ, sysStat.minCaliTemp, "minCaliBBQTemp", forceUpdate);
    updateNumberComponent(maxCaliBBQTemp, lastMaxCaliBBQ, sysStat.maxCaliTemp, "maxCaliBBQTemp", forceUpdate);
    updateNumberComponent(minCaliChuTemp, lastMinCaliChunk, sysStat.minCaliTempP, "minCaliChuTemp", forceUpdate);
    updateNumberComponent(maxCaliChuTemp, lastMaxCaliChunk, sysStat.maxCaliTempP, "maxCaliChuTemp", forceUpdate);


    // Update the last page ID
    lastPageIdCali = currentPageId;
}
