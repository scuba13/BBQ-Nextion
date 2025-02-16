#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <PubSubClient.h>
#include <WiFiClient.h>
#include "SystemStatus.h"
#include "LogHandler.h"
#include "FileSystem.h"

class MQTTHandler {
public:
    MQTTHandler(SystemStatus& status, LogHandler& logger, FileSystem& fs);
    
    // Métodos principais
    bool begin();
    void loop();
    bool reconnect();
    void publish(const char* topic, const char* payload);
    
    // Getters de status
    bool isConnected() { return _mqttClient.connected(); }
    bool isEnabled() const { return _enabled && _configured; }
    
private:
    // Referências externas
    SystemStatus& _status;
    LogHandler& _logger;
    FileSystem& _fs;
    
    // Clientes MQTT
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    
    // Estado interno
    bool _enabled;
    bool _configured;
    unsigned long _lastReconnectAttempt;
    unsigned long _lastPublish;
    
    // Constantes
    static constexpr uint32_t RECONNECT_INTERVAL = 5000;  // 5 segundos
    static constexpr uint32_t PUBLISH_INTERVAL = 30000;   // 30 segundos
    static constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 3;
    static constexpr size_t MQTT_BUFFER_SIZE = 512;
    
    // Métodos auxiliares
    bool configure();
    void setupTopics();
    void publishStatus();
    void publishTemperature();
    void publishEnergy();
    void handleCallback(char* topic, byte* payload, unsigned int length);
    
    // Tópicos MQTT
    struct Topics {
        static constexpr const char* BASE = "bbq";
        static constexpr const char* TEMPERATURE = "bbq/temperature";
        static constexpr const char* PROTEIN = "bbq/protein";
        static constexpr const char* POWER = "bbq/power";
        static constexpr const char* ENERGY = "bbq/energy";
        static constexpr const char* STATUS = "bbq/status";
        static constexpr const char* COMMAND = "bbq/command";
    };
    
    // Callback estático para PubSubClient
    static void callback(char* topic, byte* payload, unsigned int length);
    static MQTTHandler* _instance;
};

#endif
