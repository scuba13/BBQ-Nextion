#include "NextionHandler.h"

// Definição dos componentes Nextion
NexPage wifi = NexPage(0, 0, "wifi");
NexPage welcome = NexPage(1, 0, "welcome");
NexPage menu = NexPage(2, 0, "menu");
NexPage monitor = NexPage(3, 0, "monitor");
NexPage BBQTemp = NexPage(4, 0, "BBQTemp");

// Page Monitor
NexNumber bbqTempSet = NexNumber(3, 10, "bbqTempSet");
NexNumber bbqTemp = NexNumber(3, 1, "bbqTemp");
NexNumber chunkTempSet = NexNumber(3, 11, "chunkTempSet");
NexNumber chunkTemp = NexNumber(3, 2, "chunkTemp");
NexNumber bbqTempAvg = NexNumber(3, 8, "bbqTempAvg");
NexPicture relayStatePic = NexPicture(3, 9, "relayStatePic");
NexNumber active = NexNumber(3, 19, "active");

// Page BBQTemp
NexNumber setBBQTemp = NexNumber(4, 1, "setBBQTemp");
NexNumber minBBQTemp = NexNumber(4, 6, "minBBQTemp");
NexNumber maxBBQTemp = NexNumber(4, 7, "maxBBQTemp");
NexButton setBBQTempPop = NexButton(4, 8, "b0");

NexTouch *nex_listen_list[] = {
    &setBBQTempPop,
    NULL};

char buffer[100] = {0};

void setBBQTempPopCallback(void *ptr)
{
    uint16_t len;
    uint16_t number;
    NexButton *btn = (NexButton *)ptr;
    dbSerialPrintln("b0PopCallback");
    dbSerialPrint("ptr=");
    dbSerialPrintln((uint32_t)ptr); 
    memset(buffer, 0, sizeof(buffer));
    
     Serial.println("setBBQTempPopCallback");
    // uint32_t value;
    // setBBQTemp.getValue(&value);
    // int bbqTempValue = static_cast<int>(value);
    //  ((SystemStatus*)ptr)->bbqTemperature = bbqTempValue;
    // Serial.println("BBQTempValue: " + String(bbqTempValue));
}

void initNextion(SystemStatus &sysStat)
{
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // Configura a porta serial para o Nextion (pinos RX2 e TX2)
    nexInit();
    setBBQTempPop.attachPop(setBBQTempPopCallback, &setBBQTempPop);
    delay(1000);
}

void updateNextionVariables(SystemStatus &sysStat)
{
    bbqTempSet.setValue(sysStat.bbqTemperature);
    bbqTemp.setValue(sysStat.calibratedTemp);
    chunkTempSet.setValue(sysStat.proteinTemperature);
    chunkTemp.setValue(sysStat.calibratedTempP);
    bbqTempAvg.setValue(sysStat.averageTemp);
    if (sysStat.isRelayOn)
    {
        relayStatePic.setPic(0);
    }
    else
    {
        relayStatePic.setPic(6);
    }

    Serial.println("-------------------------");
    Serial.print("BBQTemp: ");
    Serial.println(sysStat.bbqTemperature);
    Serial.println("-------------------------");

    minBBQTemp.setValue(sysStat.minBBQTemp);
    maxBBQTemp.setValue(sysStat.maxBBQTemp);
}
