#ifndef ENERGY_MONITOR_H
#define ENERGY_MONITOR_H

#include "SystemStatus.h"


class EnergyMonitor {
public:
    EnergyMonitor(SystemStatus& systemStatus);
    void setup();
    void loop();

private:
    SystemStatus& systemStatus;
    float current1;
    float current2;
    float voltage;
    float power;
    float energy;
    float cost; // Custo em reais
    unsigned long previousMillis;
    bool isPhaseToPhase;

    float readCurrent(int sensorPin);
    void calculateCost();
    void detectConfiguration();
};

#endif

