#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#define NUM_SAMPLES 20
#define MOVING_AVERAGE_SIZE 180

struct CureState
{
    int currentStage = -1;            // Representa o estágio atual do processo de cura. Inicialmente, é -1.
    unsigned long stageStartTime = 0; // O tempo (millis) em que o estágio atual começou.
    bool completed = false;           // Indica se o processo de cura foi concluído.
};

struct SystemStatus
{
    int tempCalibration = 0;
    int lastPos = -1;
    int currentPos = 0;
    int bbqTemperature = 0;
    int proteinTemperature = 0;
    bool isRelayOn = false;
  //  unsigned long lastDebounceTime = 0;
  //  unsigned long lastTempUpdateMonitor = 0;
  //  unsigned long lastTempUpdateCure = 0;
   // bool monitorMode = false;
    bool cureProcessMode = false;
    float samples[MOVING_AVERAGE_SIZE];
    int sampleIndex;
    int avgNumSamples;
    bool startAverage = false;
    int averageTemp = 0;
    bool hasReachedSetTemp = false;

    CureState cureState;

    // BBQ Variables Temp
    float tempSamples[NUM_SAMPLES];
    int nextSampleIndex = 0;
    int numSamples = 0;
    int calibratedTemp;

    // Protein Variables Temp
    float tempSamplesP[NUM_SAMPLES];
    int nextSampleIndexP = 0;
    int numSamplesP = 0;
    int calibratedTempP;

    // MQTT Variables
    bool isHAAvailable;
    char mqttServer[100];
    int mqttPort;
    char mqttUser[30];
    char mqttPassword[30];
    char deviceId[20];

    // Temp Config
    int minBBQTemp;
    int maxBBQTemp;
    int minPrtTemp;
    int maxPrtTemp;
    int minCaliTemp;
    int maxCaliTemp;

    // AI
    char aiKey[128];
    char tip[256];

    // Energy Monitor
    float power = 0.0;
    float energy = 0.0;
    float cost = 0.0;
    float kWhCost = 1.0; // Valor do kWh em reais, padrão de 1 real por kWh
};

#endif
