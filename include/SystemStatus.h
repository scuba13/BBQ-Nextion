#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

#define NUM_SAMPLES 20
#define MOVING_AVERAGE_SIZE 180

struct SystemStatus
{
    int tempCalibration = 0;
    int tempCalibrationP = 0;
    int bbqTemperature = 0;
    int proteinTemperature = 0;
    bool isRelayOn = false;
    bool cureProcessMode = false;
    float samples[MOVING_AVERAGE_SIZE];
    int sampleIndex;
    int avgNumSamples;
    bool startAverage = false;
    int averageTemp = 0;
    bool hasReachedSetTemp = false;

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
    int minCaliTempP;
    int maxCaliTempP;

    // AI
    char aiKey[128];
    char tip[256];

    //Internal Temp
    int calibratedTempInternal = 0;
};

#endif
