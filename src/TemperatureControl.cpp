#include "TemperatureControl.h"
#include "PinDefinitions.h"
#include <Arduino.h>
#include <Nextion.h>

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);        // Instanciação da variável
MAX6675 thermocoupleP(MAX6675_SCK_P, MAX6675_CS_P, MAX6675_SO_P); // Instanciação da variável

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

  // Aqui, primeiro calculamos a nova temperatura calibrada
  int newCalibratedTemp = (int)round(sum / sysStat.numSamples);

  // Atualiza a temperatura calibrada na estrutura sysStat
  sysStat.calibratedTemp = newCalibratedTemp;

  return sysStat.calibratedTemp;
}

void updateRelayState(int temp, SystemStatus &sysStat)
{
  if (temp <= sysStat.bbqTemperature)
  {
    digitalWrite(RELAY_PIN, HIGH);
    sysStat.isRelayOn = true;
  }
  else if (temp > sysStat.bbqTemperature)
  {
    digitalWrite(RELAY_PIN, LOW);
    sysStat.isRelayOn = false;
  }
}

void controlTemperature(SystemStatus &sysStat)
{
  //dbSerial.println("Controlando temperatura...");
  int temp = sysStat.calibratedTemp;

  // Verifica se a temperatura atingiu o valor configurado
  if (temp >= sysStat.bbqTemperature)
  {
    sysStat.hasReachedSetTemp = true;
  }

  // Inicia o cálculo da média apenas se a temperatura configurada já foi atingida
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
}

void collectSample(SystemStatus &sysStat)
{
  static unsigned long lastSampleCollection = 0;
  unsigned long currentMillis = millis();

  if (sysStat.startAverage)
  { // Só começa a coleta de amostras se startAverage for verdadeiro
    if (currentMillis - lastSampleCollection >= 60000)
    {
      lastSampleCollection = currentMillis;

      int temp = sysStat.calibratedTemp;
      addSample(temp, sysStat);
      calculateAverage(sysStat);
    }
  }
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

  // Aqui, primeiro calculamos a nova temperatura calibrada
  int newCalibratedTempP = (int)round(sum / sysStat.numSamplesP);

  // Atualiza a temperatura calibrada na estrutura sysStat
  sysStat.calibratedTempP = newCalibratedTempP;

  return sysStat.calibratedTempP;
}

// Reset the variables
void resetSystem(SystemStatus &sysStat)
{
  dbSerial.println("System reset Started...");
 
  // Desliga o relé
  digitalWrite(RELAY_PIN, LOW);
  sysStat.isRelayOn = false;

  // Reset das temperaturas
  sysStat.bbqTemperature = 0;
  sysStat.proteinTemperature = 0;
  sysStat.tempCalibration = 0;
  sysStat.tempCalibrationP = 0; // Certifique-se de resetar também a calibração da proteína, se aplicável

  // Reset das variáveis de média
  sysStat.startAverage = false;
  sysStat.averageTemp = 0;
  sysStat.numSamples = 0;
  sysStat.numSamplesP = 0;
  sysStat.hasReachedSetTemp = false;

  // Reseta os índices de amostras
  sysStat.sampleIndex = 0;
  sysStat.nextSampleIndex = 0;
  sysStat.nextSampleIndexP = 0;

  // Apaga as amostras
  for (int i = 0; i < NUM_SAMPLES; i++)
  {
    sysStat.tempSamples[i] = 0;
    sysStat.tempSamplesP[i] = 0;
  }

  // Reset das temperaturas calibradas
  sysStat.calibratedTemp = 0;
  sysStat.calibratedTempP = 0;

  //Reset das variáveis de energia
  sysStat.power = 0.0;
  sysStat.energy = 0.0;
  sysStat.cost = 0.0;

  // Debugging statements
  dbSerial.println("System reset completed.");
}
