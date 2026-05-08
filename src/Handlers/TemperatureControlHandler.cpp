#include "Handlers/TemperatureHandler.h"
#include "PinDefinitions.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Handlers/LogHandler.h"
#include "Handlers/DiagnosticsHandler.h"
#include "SysStatMutex.h"
#include "DebugInjector.h"

extern LogHandler logHandler;
extern DiagnosticsHandler diagnostics;

OneWire oneWire(Ds18b2);
DallasTemperature sensors(&oneWire);

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);        // Instanciação da variável
MAX6675 thermocoupleP(MAX6675_SCK_P, MAX6675_CS_P, MAX6675_SO_P); // Instanciação da variável

// Cache de leituras para reduzir acessos ao hardware
static struct {
    float lastBBQTemp = 0;
    float lastProbeTemp = 0;
    float lastInternalTemp = 0;
    unsigned long lastReadTime = 0;
} tempCache;


//Internal Temp
int getCalibratedInternalTemp(SystemStatus &sysStat)
{
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  // -127 = sensor desconectado; 85 = power-on reset do DS18B20
  if (temp == DEVICE_DISCONNECTED_C || temp == 85.0f) {
      diagnostics.countSensorIntError();
      return sysStat.calibratedTempInternal; // mantém última leitura válida
  }

  sysStat.calibratedTempInternal = (int)round(temp);
  return sysStat.calibratedTempInternal;
}

// BBQ Collection Functions - Otimizado
int getCalibratedTemp(MAX6675& thermocouple, SystemStatus& sysStat) {
    if (debugInjectorIsActive()) {
        sysStat.calibratedTemp = (int)round(debugInjector.bbqTemp);
        return sysStat.calibratedTemp;
    }

    unsigned long currentTime = millis();

    // Usa cache se dentro do intervalo
    if (currentTime - tempCache.lastReadTime < TEMP_READ_INTERVAL) {
        return tempCache.lastBBQTemp;
    }

    // Valida leitura antes de usar (NaN ou fora de range = falha SPI / termopar aberto)
    float raw = thermocouple.readCelsius();
    if (isnan(raw) || raw <= 0.0f || raw > 500.0f) {
        diagnostics.countSensorBBQError();
        return sysStat.calibratedTemp; // mantém última leitura válida
    }

    float temp = raw + sysStat.tempCalibration;
    sysStat.tempSamples[sysStat.nextSampleIndex] = temp;
    sysStat.nextSampleIndex = (sysStat.nextSampleIndex + 1) % NUM_SAMPLES;
    
    if (sysStat.numSamples < NUM_SAMPLES) {
        sysStat.numSamples++;
    }

    // Cálculo otimizado da média
    float sum = 0;
    for (int i = 0; i < sysStat.numSamples; i++) {
        sum += sysStat.tempSamples[i];
    }
    
    tempCache.lastBBQTemp = round(sum / sysStat.numSamples);
    tempCache.lastReadTime = currentTime;
    sysStat.calibratedTemp = tempCache.lastBBQTemp;

    return sysStat.calibratedTemp;
}

// Protein Collection Functions
int getCalibratedTempP(MAX6675 &thermocoupleP, SystemStatus &sysStat)
{
  if (debugInjectorIsActive()) {
      sysStat.calibratedTempP = (int)round(debugInjector.proteinTemp);
      return sysStat.calibratedTempP;
  }

  float raw = thermocoupleP.readCelsius();
  if (isnan(raw) || raw <= 0.0f || raw > 500.0f) {
      diagnostics.countSensorPrtError();
      return sysStat.calibratedTempP; // mantém última leitura válida
  }

  float temp = raw + sysStat.tempCalibrationP;
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

void controlTemperature(SystemStatus& sysStat) {
    int temp = sysStat.calibratedTemp;

    // Failsafe absoluto: temperatura acima do limite seguro → relé desligado imediatamente
    if (temp >= MAX_SAFE_TEMP) {
        if (sysStat.isRelayOn) {
            digitalWrite(RELAY_PIN, LOW);
            sysStat.isRelayOn = false;
            diagnostics.countRelayEmergency();
            logHandler.logError("EMERGENCIA: temp " + String(temp) +
                                "C acima do limite de " + String(MAX_SAFE_TEMP) + "C — relé desligado!");
        }
        return; // não executa a lógica normal
    }

    if (temp >= sysStat.bbqTemperature) {
        sysStat.hasReachedSetTemp = true;
        sysStat.startAverage = true;
    }

    // Histerese real: OFF ao atingir setpoint, ON apenas quando cair HYSTERESIS abaixo
    // Zona neutra entre (setpoint - HYSTERESIS) e setpoint mantém o estado atual
    if (temp >= sysStat.bbqTemperature) {
        digitalWrite(RELAY_PIN, LOW);
        sysStat.isRelayOn = false;
    }
    else if (temp < sysStat.bbqTemperature - TEMP_HYSTERESIS) {
        digitalWrite(RELAY_PIN, HIGH);
        sysStat.isRelayOn = true;
    }

    collectSample(sysStat);

    // Detecta transição: proteína atingiu setpoint (loga e publica uma única vez)
    if (sysStat.proteinTemperature > 0 &&
        !sysStat.proteinReached &&
        sysStat.calibratedTempP >= sysStat.proteinTemperature) {
        sysStat.proteinReached = true;
        logHandler.logMessage("Proteína atingiu temperatura alvo: " +
                              String(sysStat.calibratedTempP) + "C / setpoint: " +
                              String(sysStat.proteinTemperature) + "C");
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

  sysStatLock();
  digitalWrite(RELAY_PIN, LOW);
  sysStat.isRelayOn = false;

  sysStat.bbqTemperature = 0;
  sysStat.proteinTemperature = 0;
  // C-02: calibração é config de hardware — não reseta com o sistema

  sysStat.startAverage = false;
  sysStat.averageTemp = 0;
  sysStat.numSamples = 0;
  sysStat.numSamplesP = 0;
  sysStat.hasReachedSetTemp = false;
  sysStat.proteinReached = false;

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

  digitalWrite(RGB_BUILTIN, LOW);
  sysStatUnlock();

  logHandler.logMessage("System reset completed.");
}
