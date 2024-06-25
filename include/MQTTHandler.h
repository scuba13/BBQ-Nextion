#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "SystemStatus.h"
#include <ArduinoJson.h>
#include "LogHandler.h"

class MQTTHandler
{
public:
    MQTTHandler(WiFiClient &net, PubSubClient &client, SystemStatus &systemStatus, LogHandler &logger); // Modifique o construtor
    void connectMQTT();
    void publishAllMessages(SystemStatus &systemStatus);
    void checkAndReconnectAwsIoT();
    void messageHandler(char *topic, byte *payload, unsigned int length);
    void verifyAndReconnect(SystemStatus &systemStatus);
    void managePublishing(SystemStatus &systemStatus);

private:
    WiFiClient &net;      // Usando referência
    PubSubClient &client; // Usando referência
    SystemStatus &systemStatus;
    LogHandler &_logger;

    void publishMessageBBQTemp(SystemStatus &systemStatus);
    void publishMessageBBQTempSet(SystemStatus &systemStatus);
    void publishMessageBBQProtein(SystemStatus &systemStatus);
    void publishMessageRelayStatus(SystemStatus &systemStatus);
    void publishMessageAvgTemp(SystemStatus &systemStatus);
};

#endif // MQTT_HANDLER_H
