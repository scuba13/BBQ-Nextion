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

static float lastPower = 0;
static float lastEnergy = 0;
static float lastCost = 0;
uint32_t lastPageIdEnergy = -1;


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
NexButton stop = NexButton(3, 18, "stop");

// Page BBQTemp
NexNumber setBBQTemp = NexNumber(4, 1, "setBBQTemp");
NexNumber minBBQTemp = NexNumber(4, 6, "minBBQTemp");
NexNumber maxBBQTemp = NexNumber(4, 7, "maxBBQTemp");
NexButton setBBQTempPop = NexButton(4, 8, "setBBQ");

// Page ChunkTemp
NexNumber setChunkTemp = NexNumber(5, 1, "setChunkTemp");
NexNumber minChunkTemp = NexNumber(5, 6, "minChunkTemp");
NexNumber maxChunkTemp = NexNumber(5, 7, "maxChunkTemp");
NexButton setChunkTempPop = NexButton(5, 8, "setChunk");


// Page Energy
NexNumber power = NexNumber(6, 2, "power");
NexNumber energy = NexNumber(6, 3, "energy");
NexNumber cost = NexNumber(6, 4, "cost");

NexTouch *nex_listen_list[] = {
    &setBBQTempPop,
    &setChunkTempPop,
    &stop,
    NULL};

char buffer[100] = {0};

void setBBQTempPopCallback(void *ptr)
{
    Serial.println("Entering setBBQTempPopCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        Serial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    // Get the value from setBBQTemp
    uint32_t value;
    bool success = setBBQTemp.getValue(&value);

    if (!success)
    {
        Serial.println("Error: Failed to get value from setBBQTemp");
        return;
    }

    // Cast value to int and set it to the bbqTemperature
    int bbqTempValue = static_cast<int>(value);
    systemStatus->bbqTemperature = bbqTempValue;

    // Debugging statements
    Serial.println("BBQTempValue: " + String(bbqTempValue));
    Serial.println("SystemStatus BBQ Temperature: " + String(systemStatus->bbqTemperature));
    Serial.println("Exiting setBBQTempPopCallback");
    monitor.show();
}

void setChunkTempPopCallback(void *ptr)
{
    Serial.println("Entering setChunkTempPopCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        Serial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    // Get the value from setBBQTemp
    uint32_t value;
    bool success = setChunkTemp.getValue(&value);

    if (!success)
    {
        Serial.println("Error: Failed to get value from setChunkTemp");
        return;
    }

    // Cast value to int and set it to the bbqTemperature
    int chunkTempValue = static_cast<int>(value);
    systemStatus->proteinTemperature = chunkTempValue;

    // Debugging statements
    Serial.println("BBQTempValue: " + String(chunkTempValue));
    Serial.println("SystemStatus Chunk Temperature: " + String(systemStatus->proteinTemperature));
    Serial.println("Exiting setChunkTempPopCallback");
    monitor.show();
}

void setStopPushCallback(void *ptr)
{
    Serial.println("Entering setStopPushCallback");

    // Check if the pointer is valid
    if (ptr == nullptr)
    {
        Serial.println("Error: ptr is null");
        return;
    }

    // Cast the pointer to SystemStatus
    SystemStatus *systemStatus = static_cast<SystemStatus *>(ptr);

    resetSystem(*systemStatus);

    Serial.println("Exiting setStopPushCallback");
}

void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // Configura a porta serial para o Nextion (pinos RX2 e TX2)
    nexInit();
    setBBQTempPop.attachPop(setBBQTempPopCallback, &sysStat);
    setChunkTempPop.attachPop(setChunkTempPopCallback, &sysStat);
    stop.attachPush(setStopPushCallback, &sysStat);

    delay(1000);
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

    // Serial.print("Current Page ID: ");
    // Serial.println(pageId);

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
    // Serial.println("Command: " + cmd);
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
        Serial.print("Updating ");
        Serial.print(componentName);
        Serial.print(" from ");
        Serial.print(lastValue);
        Serial.print(" to ");
        Serial.println(newValue);

        component.setValue(static_cast<uint32_t>(newValue));
        lastValue = newValue;
    }
    else
    {
        Serial.print("No update needed for ");
        Serial.print(componentName);
        Serial.print(", value remains ");
        Serial.println(lastValue);
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

    Serial.println("-------------------------");
    updateNumberComponent(bbqTempSet, lastBbqTempSet, sysStat.bbqTemperature, "bbqTempSet", forceUpdate);
    updateNumberComponent(bbqTemp, lastBbqTemp, sysStat.calibratedTemp, "bbqTemp", forceUpdate);
    updateNumberComponent(chunkTempSet, lastChunkTempSet, sysStat.proteinTemperature, "chunkTempSet", forceUpdate);
    updateNumberComponent(chunkTemp, lastChunkTemp, sysStat.calibratedTempP, "chunkTemp", forceUpdate);
    updateNumberComponent(bbqTempAvg, lastBbqTempAvg, sysStat.averageTemp, "bbqTempAvg", forceUpdate);

    if (sysStat.isRelayOn != lastRelayState || forceUpdate)
    {
        Serial.print("Updating relay state from ");
        Serial.print(lastRelayState);
        Serial.print(" to ");
        Serial.println(sysStat.isRelayOn);

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
        Serial.print("No update needed for relay state, value remains ");
        Serial.println(lastRelayState);
    }

    Serial.println("-------------------------");

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

    Serial.println("-------------------------");
    updateNumberComponent(minBBQTemp, lastMinBBQTemp, sysStat.minBBQTemp, "minBBQTemp", forceUpdate);
    updateNumberComponent(maxBBQTemp, lastMaxBBQTemp, sysStat.maxBBQTemp, "maxBBQTemp", forceUpdate);
    Serial.println("-------------------------");

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

    Serial.println("-------------------------");
    updateNumberComponent(minChunkTemp, lastMinChunkTemp, sysStat.minPrtTemp, "minChunkTemp", forceUpdate);
    updateNumberComponent(maxChunkTemp, lastMaxChunkTemp, sysStat.maxPrtTemp, "maxChunkTemp", forceUpdate);
    Serial.println("-------------------------");

    // Update the last page ID
    lastPageIdChunk = currentPageId;
}

void updateNextionEnergyVariables(SystemStatus &sysStat)
{
    uint32_t currentPageId = getCurrentPageId();
    bool forceUpdate = (currentPageId != lastPageIdEnergy); // Force update if the page has changed

    // Check if the current page is the energy page
    if (currentPageId != 6)
    {
        lastPageIdEnergy = currentPageId;
        return;
    }

    Serial.println("-------------------------");

    // Ensure the values are not negative
    float powerValue = sysStat.power > 0 ? sysStat.power : 0;
    // Serial.print("Power Value: ");
    // Serial.println(powerValue);
    float energyValue = sysStat.energy > 0 ? sysStat.energy : 0;
    // Serial.print("Energy Value: ");
    // Serial.println(energyValue);
    float costValue = sysStat.cost > 0 ? sysStat.cost : 0;
    // Serial.print("Cost Value: ");
    // Serial.println(costValue);

    updateNumberComponent(power, lastPower, powerValue, "power", forceUpdate);
    updateNumberComponent(energy, lastEnergy, energyValue, "energy", forceUpdate);
    updateNumberComponent(cost, lastCost, costValue, "cost", forceUpdate);

    Serial.println("-------------------------");

    // Update the last page ID
    lastPageIdEnergy = currentPageId;
}
