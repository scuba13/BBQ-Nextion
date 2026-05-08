#ifndef TEMPERATURE_CONTROL_H
#define TEMPERATURE_CONTROL_H

#include "SystemStatus.h"
#include "Config.h"
#include <max6675.h>
#include <OneWire.h>
#include <DallasTemperature.h>

extern MAX6675 thermocouple; // Declaração externa da variável
extern MAX6675 thermocoupleP; // Declaração externa da variável

int getCalibratedTemp(MAX6675& thermocouple, SystemStatus& sysStat);
int getCalibratedTempP(MAX6675& thermocoupleP, SystemStatus& sysStat);
void controlTemperature(SystemStatus& sysStat);
void resetSystem(SystemStatus& sysStat);
void calculateMovingAverage(int temp, SystemStatus& sysStat);
void collectSample(SystemStatus& sysStat);
void resetSystem(SystemStatus &sysStat);
int getCalibratedInternalTemp(SystemStatus &sysStat);



#endif
