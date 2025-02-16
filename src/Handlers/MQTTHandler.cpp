#include "MQTTHandler.h"
#include "TemperatureControl.h"
#include <Nextion.h>
#include <ArduinoJson.h>

extern LogHandler _logger;

MQTTHandler* MQTTHandler::_instance = nullptr;

MQTTHandler::MQTTHandler(SystemStatus& status, LogHandler& logger, FileSystem& fs)
    : _status(status)
    , _logger(logger)
    , _fs(fs)
    , _mqttClient(_wifiClient)
    , _enabled(false)
    , _configured(false)
    , _lastReconnectAttempt(0)
    , _lastPublish(0)
{
    _instance = this;
    _mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    _mqttClient.setCallback(callback);
    begin();  // Inicializar automaticamente
}

bool MQTTHandler::begin() {
    if (!configure()) {
        _logger.logError("MQTT: Configuration failed");
        return false;
    }
    
    setupTopics();
    _enabled = true;
    return true;
}

void MQTTHandler::loop() {
    if (!_enabled || !_configured) return;

    if (!_mqttClient.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > RECONNECT_INTERVAL) {
            _lastReconnectAttempt = now;
            if (reconnect()) {
                _lastReconnectAttempt = 0;
            }
        }
    } else {
        _mqttClient.loop();
        
        // Publicar atualizações periódicas
        unsigned long now = millis();
        if (now - _lastPublish > PUBLISH_INTERVAL) {
            _lastPublish = now;
            publishStatus();
        }
    }
}

bool MQTTHandler::reconnect() {
    static uint8_t attempts = 0;
    
    if (attempts >= MAX_RECONNECT_ATTEMPTS) {
        _logger.logError("MQTT: Max reconnection attempts reached");
        _enabled = false;
        attempts = 0;
        return false;
    }
    
    _logger.logMessage("MQTT: Attempting connection...");
    
    String clientId = "BBQ-";
    clientId += String(random(0xffff), HEX);
    
    if (_mqttClient.connect(clientId.c_str(), _status.mqttUser, _status.mqttPassword)) {
        _logger.logMessage("MQTT: Connected");
        _mqttClient.subscribe(Topics::COMMAND);
        attempts = 0;
        return true;
    }
    
    attempts++;
    _logger.logError("MQTT: Connection failed, rc=" + String(_mqttClient.state()));
    return false;
}

void MQTTHandler::publish(const char* topic, const char* payload) {
    if (!isConnected()) return;
    
    if (_mqttClient.publish(topic, payload)) {
        _logger.logMessage("MQTT: Published to " + String(topic));
    } else {
        _logger.logError("MQTT: Failed to publish to " + String(topic));
    }
}

bool MQTTHandler::configure() {
    if (_status.mqttServer[0] == '\0' || _status.mqttPort == 0) {
        return false;
    }
    
    _mqttClient.setServer(_status.mqttServer, _status.mqttPort);
    _configured = true;
    return true;
}

void MQTTHandler::setupTopics() {
    // Configurar tópicos MQTT aqui se necessário
}

void MQTTHandler::publishStatus() {
    DynamicJsonDocument doc(256);
    doc["bbq_temp"] = _status.calibratedTemp;
    doc["protein_temp"] = _status.calibratedTempP;
    doc["target_temp"] = _status.bbqTemperature;
    doc["relay"] = _status.isRelayOn;
    
    String output;
    serializeJson(doc, output);
    publish(Topics::STATUS, output.c_str());
    
    publishTemperature();
    publishEnergy();
}

void MQTTHandler::publishTemperature() {
    publish(Topics::TEMPERATURE, String(_status.calibratedTemp).c_str());
    publish(Topics::PROTEIN, String(_status.calibratedTempP).c_str());
}

void MQTTHandler::publishEnergy() {
    DynamicJsonDocument doc(128);
    doc["power"] = _status.power;
    doc["energy"] = _status.energy;
    doc["cost"] = _status.cost;
    
    String output;
    serializeJson(doc, output);
    publish(Topics::ENERGY, output.c_str());
}

void MQTTHandler::callback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->handleCallback(topic, payload, length);
    }
}

void MQTTHandler::handleCallback(char* topic, byte* payload, unsigned int length) {
    // Converter payload para string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    _logger.logMessage("MQTT: Message received [" + String(topic) + "] " + String(message));
    
    if (strcmp(topic, Topics::COMMAND) == 0) {
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, message);
        
        if (error) {
            _logger.logError("MQTT: JSON parsing failed");
            return;
        }
        
        // Processar comandos aqui
        if (doc.containsKey("target_temp")) {
            _status.bbqTemperature = doc["target_temp"];
        }
    }
}
