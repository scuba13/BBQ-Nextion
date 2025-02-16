#include "TemperatureControl.h"
#include "PinDefinitions.h"
#include <Arduino.h>
#include <Nextion.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "AppContext.h"

OneWire oneWire(Ds18b2);
DallasTemperature sensors(&oneWire);

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);        // Instanciação da variável
MAX6675 thermocoupleP(MAX6675_SCK_P, MAX6675_CS_P, MAX6675_SO_P); // Instanciação da variável

// Cache para ambos os sensores
static struct {
    unsigned long lastRead = 0;
    float lastTemp = 0;
    float lastTempP = 0;
} sensorCache;

const unsigned long TEMP_READ_INTERVAL = 250; // 4 leituras por segundo

// Otimizar leitura dos sensores
float readTemperature(MAX6675 &thermocouple, bool isProtein = false) {
    unsigned long now = millis();
    if (now - sensorCache.lastRead >= TEMP_READ_INTERVAL) {
        sensorCache.lastRead = now;
        if (isProtein) {
            sensorCache.lastTempP = thermocouple.readCelsius();
            return sensorCache.lastTempP;
        } else {
            sensorCache.lastTemp = thermocouple.readCelsius();
            return sensorCache.lastTemp;
        }
    }
    return isProtein ? sensorCache.lastTempP : sensorCache.lastTemp;
}

//Internal Temp
int getCalibratedInternalTemp(SystemStatus &sysStat)
{
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  sysStat.calibratedTempInternal = (int)round(temp);

  return sysStat.calibratedTempInternal;
}

// Otimizar cálculo de média
float calculateMovingAverage(float samples[], int count) {
    static float sum = 0;
    static int lastCount = 0;
    static float lastAverage = 0;
    
    // Se não houver mudança no número de amostras, retorna último cálculo
    if (count == lastCount) {
        return lastAverage;
    }
    
    sum = 0;
    for (int i = 0; i < count; i++) {
        sum += samples[i];
    }
    
    lastCount = count;
    lastAverage = sum / count;
    return lastAverage;
}

// BBQ Collection Functions otimizada
int getCalibratedTemp(MAX6675 &thermocouple, SystemStatus &sysStat) {
    float temp = readTemperature(thermocouple) + sysStat.tempCalibration;
    
    // Atualizar buffer circular
    sysStat.tempSamples[sysStat.nextSampleIndex] = temp;
    sysStat.nextSampleIndex = (sysStat.nextSampleIndex + 1) % NUM_SAMPLES;
    sysStat.numSamples = min(sysStat.numSamples + 1, NUM_SAMPLES);
    
    // Usar função otimizada de média
    sysStat.calibratedTemp = (int)round(calculateMovingAverage(sysStat.tempSamples, sysStat.numSamples));
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

  //logHandler.logMessage("Calibrated TempP: " + String(sysStat.calibratedTempP));

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
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);  // Blue
    sysStat.isRelayOn = false;
  }

  //logHandler.logMessage("Relay state updated: " + String(sysStat.isRelayOn ? "ON" : "OFF"));
}

// Otimizar controle de temperatura
void controlTemperature(SystemStatus &sysStat) {
    static unsigned long lastCheck = 0;
    static bool wasAboveTarget = false;
    const unsigned long checkInterval = 100; // 100ms
    
    if (millis() - lastCheck < checkInterval) return;
    lastCheck = millis();
    
    const float hysteresis = 1.0;
    int temp = sysStat.calibratedTemp;
    bool isAboveTarget = temp > sysStat.bbqTemperature;
    
    // Reduzir chaveamento usando estado anterior
    if (isAboveTarget != wasAboveTarget) {
        if (isAboveTarget) {
            if (temp > sysStat.bbqTemperature + hysteresis) {
                digitalWrite(RELAY_PIN, LOW);
                neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS);
                sysStat.isRelayOn = false;
            }
        } else {
            if (temp < sysStat.bbqTemperature - hysteresis) {
                digitalWrite(RELAY_PIN, HIGH);
                neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0);
                sysStat.isRelayOn = true;
            }
        }
        wasAboveTarget = isAboveTarget;
    }
    
    // Atualizar estados apenas na primeira vez que atinge a temperatura
    if (!sysStat.hasReachedSetTemp && temp >= sysStat.bbqTemperature) {
        sysStat.hasReachedSetTemp = true;
        sysStat.startAverage = true;
    }
    
    // Otimizar coleta de amostras
    if (sysStat.startAverage) {
        collectSample(sysStat);
    }
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

void calculateAverage(SystemStatus &sysStat) {
    float sum = 0;
    for (int i = 0; i < sysStat.avgNumSamples; i++) {
        sum += sysStat.samples[i];
    }
    sysStat.averageTemp = sum / sysStat.avgNumSamples;
    app.logHandler.logMessage("Average Temp: " + String(sysStat.averageTemp));
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
  app.logHandler.logMessage("System Reset Started...");

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

  app.logHandler.logMessage("System reset completed.");
}
