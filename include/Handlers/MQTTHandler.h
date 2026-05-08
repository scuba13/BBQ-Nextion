#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "SystemStatus.h"
#include "Handlers/LogHandler.h"

class MQTTHandler
{
public:
    MQTTHandler(WiFiClient &net, PubSubClient &client, SystemStatus &systemStatus, LogHandler &logger);
    
    // Métodos públicos otimizados
    void begin(const char* server, int port, const char* user = nullptr, const char* password = nullptr);
    void loop();
    void publishTemperature(float temp, float tempP);
    void publishStatus(const String& status);
    void messageHandler(char *topic, byte *payload, unsigned int length);
    void publishAllMessages(SystemStatus &systemStatus);
    void checkAndReconnectAwsIoT();
    void verifyAndReconnect(SystemStatus &systemStatus);
    void managePublishing(SystemStatus &systemStatus);
    bool isConnected();

private:
    WiFiClient &net;      // Usando referência
    PubSubClient &client; // Usando referência
    SystemStatus &systemStatus;
    LogHandler &_logger;

    // Credenciais MQTT
    struct {
        String user;
        String password;
    } credentials;

    // Métodos privados
    void handleCallback(char* topic, byte* payload, unsigned int length);
    void subscribeToTopics();
    bool connect();
    void publish(const char* topic, const String& message);
    void processMessage(const String& topic, const String& payload);

    void publishMessageBBQTemp(SystemStatus &systemStatus);
    void publishMessageBBQTempSet(SystemStatus &systemStatus);
    void publishMessageBBQProtein(SystemStatus &systemStatus);
    void publishMessageRelayStatus(SystemStatus &systemStatus);
    void publishMessageAvgTemp(SystemStatus &systemStatus);
};

#endif // MQTT_HANDLER_H
