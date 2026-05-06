#include "MQTTHandler.h"
#include "TemperatureControl.h"
#include "LogHandler.h"

extern LogHandler logHandler;

// Configurações otimizadas
#define MQTT_BUFFER_SIZE 1024
#define MQTT_KEEP_ALIVE 30
#define MQTT_QOS 1
#define MQTT_RETRY_INTERVAL 5000
#define MAX_PAYLOAD_SIZE 256

// Cache de mensagens
static struct {
    String lastTemp;
    String lastStatus;
    unsigned long lastPublish = 0;
    const unsigned long PUBLISH_INTERVAL = 1000; // 1 segundo entre publicações
} mqttCache;

MQTTHandler::MQTTHandler(WiFiClient& net, PubSubClient& client, SystemStatus& systemStatus, LogHandler& logger)
    : net(net), client(client), systemStatus(systemStatus), _logger(logger) {
    // Construtor
}

void MQTTHandler::begin(const char* server, int port, const char* user, const char* password) {
    client.setBufferSize(MQTT_BUFFER_SIZE);
    client.setKeepAlive(MQTT_KEEP_ALIVE);
    
    // Configurações de conexão
    client.setServer(server, port);
    
    // Atualiza credenciais do sistema usando strcpy
    strncpy(systemStatus.mqttServer, server, sizeof(systemStatus.mqttServer) - 1);
    systemStatus.mqttServer[sizeof(systemStatus.mqttServer) - 1] = '\0';
    
    systemStatus.mqttPort = port;
    
    if (user) {
        strncpy(systemStatus.mqttUser, user, sizeof(systemStatus.mqttUser) - 1);
        systemStatus.mqttUser[sizeof(systemStatus.mqttUser) - 1] = '\0';
    }
    
    if (password) {
        strncpy(systemStatus.mqttPassword, password, sizeof(systemStatus.mqttPassword) - 1);
        systemStatus.mqttPassword[sizeof(systemStatus.mqttPassword) - 1] = '\0';
    }
    
    // Salva credenciais locais
    if (user && password) {
        credentials.user = user;
        credentials.password = password;
    }
    
    // Callback otimizado
    client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->handleCallback(topic, payload, length);
    });
    
    logHandler.logMessage("MQTT inicializado - Servidor: " + String(server) + ":" + String(port));
}

bool MQTTHandler::connect() {
    if (client.connected()) return true;
    
    static unsigned long lastAttempt = 0;
    unsigned long now = millis();
    
    if (now - lastAttempt < MQTT_RETRY_INTERVAL) {
        return false;
    }
    lastAttempt = now;
    
    logHandler.logMessage("Conectando ao MQTT Broker...");
    
    String clientId = String(systemStatus.deviceId).length() > 0
        ? "BBQ-" + String(systemStatus.deviceId)
        : "BBQ-" + String(random(0xffff), HEX);
    
    bool connected = credentials.user.length() > 0 ?
        client.connect(clientId.c_str(), credentials.user.c_str(), credentials.password.c_str()) :
        client.connect(clientId.c_str());
    
    if (connected) {
        logHandler.logMessage("Conectado ao MQTT Broker");
        subscribeToTopics();
    } else {
        logHandler.logMessage("Falha ao conectar ao MQTT Broker, rc=" + String(client.state()));
    }
    
    return connected;
}

void MQTTHandler::publish(const char* topic, const String& message) {
    if (!client.connected() || !connect()) return;
    
    // Verifica intervalo mínimo entre publicações
    unsigned long now = millis();
    if (now - mqttCache.lastPublish < mqttCache.PUBLISH_INTERVAL) return;
    mqttCache.lastPublish = now;
    
    // Limita tamanho do payload
    String payload = message;
    if (payload.length() > MAX_PAYLOAD_SIZE) {
        payload = payload.substring(0, MAX_PAYLOAD_SIZE);
    }
    
    client.publish(topic, payload.c_str(), MQTT_QOS);
}

void MQTTHandler::publishTemperature(float temp, float tempP) {
    String newTemp = String(temp) + "|" + String(tempP);
    if (newTemp == mqttCache.lastTemp) return; // Evita publicação redundante
    
    mqttCache.lastTemp = newTemp;
    publish("bbq/temperature", newTemp);
}

void MQTTHandler::publishStatus(const String& status) {
    if (status == mqttCache.lastStatus) return; // Evita publicação redundante
    
    mqttCache.lastStatus = status;
    publish("bbq/status", status);
}

void MQTTHandler::processMessage(const String& topic, const String& payload) {
    char topicBuf[topic.length() + 1];
    char payloadBuf[payload.length() + 1];
    topic.toCharArray(topicBuf, sizeof(topicBuf));
    payload.toCharArray(payloadBuf, sizeof(payloadBuf));
    messageHandler(topicBuf, (byte*)payloadBuf, payload.length());
}

void MQTTHandler::handleCallback(char* topic, byte* payload, unsigned int length) {
    // Buffer estático para payload
    static char message[MAX_PAYLOAD_SIZE + 1];
    length = min(length, (unsigned int)MAX_PAYLOAD_SIZE);
    
    memcpy(message, payload, length);
    message[length] = '\0';
    
    String topicStr = String(topic);
    String payloadStr = String(message);
    
    // Processa mensagem
    processMessage(topicStr, payloadStr);
}

void MQTTHandler::loop() {
    if (!client.connected() && !connect()) {
        return;
    }
    client.loop();
}

void MQTTHandler::subscribeToTopics() {
    client.subscribe("bbq/command", MQTT_QOS);
    client.subscribe("bbq/config", MQTT_QOS);
}

void MQTTHandler::messageHandler(char* topic, byte* payload, unsigned int length) {
    String receivedTopic = String(topic);
    String messageTemp = String((char*)payload, length);
    logHandler.logMessage("Received [" + receivedTopic + "]: " + messageTemp);

    // Verifica se a mensagem é para setar a temperatura da BBQ
    if (receivedTopic == "sensor/bbq_set_temperature/set") {
        float bbqTempSet = messageTemp.toFloat();
        if (bbqTempSet >= 30 && bbqTempSet <= 250) {
            systemStatus.bbqTemperature = bbqTempSet;
            logHandler.logMessage("Nova temperatura da BBQ setada: " + String(systemStatus.bbqTemperature));
        } else {
            logHandler.logMessage("Valor de BBQTempSet inválido!");
        }
    }
    // Verifica se a mensagem é para setar a temperatura da proteína
    else if (receivedTopic == "sensor/protein_temperature/set") {
        float proteinTempSet = messageTemp.toFloat();
        if (proteinTempSet >= 25 && proteinTempSet <= 100) {
            systemStatus.proteinTemperature = proteinTempSet;
            logHandler.logMessage("Nova temperatura da proteína setada: " + String(systemStatus.proteinTemperature));
        } else {
            logHandler.logMessage("Valor de ProteinTempSet inválido!");
        }
    }
    // Verifica se a mensagem é para resetar o sistema
    else if (receivedTopic == "func/reset_cmd") {
        // Executa a lógica de reset apenas se o payload for "reset"
        if (messageTemp == "RESET") {
            resetSystem(systemStatus);
            logHandler.logMessage("Sistema Resetado");
        }
    }
}

void MQTTHandler::publishAllMessages(SystemStatus& systemStatus) {
    auto pub = [this](const char* topic, const String& val) {
        if (!client.publish(topic, val.c_str())) {
            logHandler.logError("Falha ao publicar MQTT: " + String(topic));
        }
    };

    pub("sensor/bbq_temperature",         String(systemStatus.calibratedTemp));
    pub("sensor/bbq_set_temperature",     String(systemStatus.bbqTemperature));
    pub("relay/state",                    systemStatus.isRelayOn ? "electric" : "off");
    pub("sensor/protein_temperature",     String(systemStatus.calibratedTempP));
    pub("sensor/protein_set_temperature", String(systemStatus.proteinTemperature));
    pub("relay_protein/state",            systemStatus.proteinTemperature > 0 ? "electric" : "off");
    pub("sensor/bbq_average_temperature", String(systemStatus.averageTemp));
}

void MQTTHandler::checkAndReconnectAwsIoT() {
    if (!client.connected()) {
        connect();
    }
    client.loop();
}

void MQTTHandler::verifyAndReconnect(SystemStatus& systemStatus) {
    if (systemStatus.isHAAvailable) {
        logHandler.logMessage("MQTT Broker disponível");
        if (!client.connected()) {
            connect();
        }
    } else {
        logHandler.logMessage("MQTT Broker não disponível");
    }
}

void MQTTHandler::managePublishing(SystemStatus& systemStatus) {
    if (client.connected()) {
        publishAllMessages(systemStatus);
    } else {
        if (connect()) {
            publishAllMessages(systemStatus);
        } else {
            logHandler.logMessage("Falha ao reconectar ao MQTT");
        }
    }
}
