#include "TemperatureControl.h"
#include "PinDefinitions.h"
#include <Arduino.h>
#include <Nextion.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "LogHandler.h"

extern LogHandler logHandler; // Certifique-se de que o logHandler esteja declarado externamente ou passado como argumento

OneWire oneWire(Ds18b2);
DallasTemperature sensors(&oneWire);

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);        // Instanciação da variável
MAX6675 thermocoupleP(MAX6675_SCK_P, MAX6675_CS_P, MAX6675_SO_P); // Instanciação da variável

//Internal Temp
int getCalibratedInternalTemp(SystemStatus &sysStat)
{
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  sysStat.calibratedTempInternal = (int)round(temp);

  return sysStat.calibratedTempInternal;
}

// BBQ Collection Functions
int getCalibratedTemp(MAX6675 &thermocouple, SystemStatus &sysStat)
{
  float temp = thermocouple.readCelsius() + sysStat.tempCalibration;
  sysStat.tempSamples[sysStat.nextSampleIndex] = temp;
  sysStat.nextSampleIndex = (sysStat.nextSampleIndex + 1) % NUM_SAMPLES;
  if (sysStat.numSamples < NUM_SAMPLES)
  {
    sysStat.numSamples++;
  }

  float sum = 0;
  for (int i = 0; i < sysStat.numSamples; i++)
  {
    sum += sysStat.tempSamples[i];
  }

  int newCalibratedTemp = (int)round(sum / sysStat.numSamples);
  sysStat.calibratedTemp = newCalibratedTemp;

  logHandler.logMessage("Calibrated Temp: " + String(sysStat.calibratedTemp));

  return sysStat.calibratedTemp;
}

// Protein Collection Functions
int getCalibratedTempP(MAX6675 &thermocoupleP, SystemStatus &sysStat)
{
  float temp = thermocoupleP.readCelsius() + sysStat.tempCalibrationP;
  sysStat.tempSamplesP[sysStat.nextSampleIndexP] = temp;
  sysStat.nextSampleIndexP = (sysStat.nextSampleIndexP + 1) % NUM_SAMPLES;
  if (sysStat.numSamplesP < NUM_SAMPLES)
  {
    sysStat.numSamplesP++;
  }

  float sum = 0;
  for (int i = 0; i < sysStat.numSamplesP; i++)
  {
    sum += sysStat.tempSamplesP[i];
  }

  int newCalibratedTempP = (int)round(sum / sysStat.numSamplesP);
  sysStat.calibratedTempP = newCalibratedTempP;

  logHandler.logMessage("Calibrated TempP: " + String(sysStat.calibratedTempP));

  return sysStat.calibratedTempP;
}

void updateRelayState(int temp, SystemStatus &sysStat)
{
  if (temp <= sysStat.bbqTemperature)
  {
    digitalWrite(RELAY_PIN, HIGH);
    neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);  // Red
    sysStat.isRelayOn = true;
  }
  else if (temp > sysStat.bbqTemperature)
  {
    digitalWrite(RELAY_PIN, LOW);
   // neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTINESS);  // Blue
    sysStat.isRelayOn = false;
  }

  logHandler.logMessage("Relay state updated: " + String(sysStat.isRelayOn ? "ON" : "OFF"));
}

void controlTemperature(SystemStatus &sysStat)
{
  logHandler.logMessage("Controlando temperatura...");
  int temp = sysStat.calibratedTemp;

  if (temp >= sysStat.bbqTemperature)
  {
    sysStat.hasReachedSetTemp = true;
  }

  if (sysStat.hasReachedSetTemp)
  {
    sysStat.startAverage = true;
  }

  updateRelayState(temp, sysStat);
  collectSample(sysStat);
}

void addSample(int temp, SystemStatus &sysStat)
{
  sysStat.samples[sysStat.sampleIndex] = temp;
  sysStat.sampleIndex = (sysStat.sampleIndex + 1) % MOVING_AVERAGE_SIZE;
  if (sysStat.avgNumSamples < MOVING_AVERAGE_SIZE)
  {
    sysStat.avgNumSamples++;
  }
}

void calculateAverage(SystemStatus &sysStat)
{
  float sum = 0;
  for (int i = 0; i < sysStat.avgNumSamples; i++)
  {
    sum += sysStat.samples[i];
  }
  sysStat.averageTemp = sum / sysStat.avgNumSamples;
  logHandler.logMessage("Average Temp: " + String(sysStat.averageTemp));
}

void collectSample(SystemStatus &sysStat)
{
  static unsigned long lastSampleCollection = 0;
  unsigned long currentMillis = millis();

  if (sysStat.startAverage)
  {
    if (currentMillis - lastSampleCollection >= 60000)
    {
      lastSampleCollection = currentMillis;

      int temp = sysStat.calibratedTemp;
      addSample(temp, sysStat);
      calculateAverage(sysStat);
    }
  }
}

void resetSystem(SystemStatus &sysStat)
{
  logHandler.logMessage("System Reset Started...");

  digitalWrite(RELAY_PIN, LOW);
  sysStat.isRelayOn = false;

  sysStat.bbqTemperature = 0;
  sysStat.proteinTemperature = 0;
  sysStat.tempCalibration = 0;
  sysStat.tempCalibrationP = 0;

  sysStat.startAverage = false;
  sysStat.averageTemp = 0;
  sysStat.numSamples = 0;
  sysStat.numSamplesP = 0;
  sysStat.hasReachedSetTemp = false;

  sysStat.sampleIndex = 0;
  sysStat.nextSampleIndex = 0;
  sysStat.nextSampleIndexP = 0;

  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    sysStat.tempSamples[i] = 0;
    sysStat.tempSamplesP[i] = 0;
  }

  sysStat.calibratedTemp = 0;
  sysStat.calibratedTempP = 0;

  sysStat.power = 0.0;
  sysStat.energy = 0.0;
  sysStat.cost = 0.0;

  digitalWrite(RGB_BUILTIN, LOW);

  logHandler.logMessage("System reset completed.");
}
