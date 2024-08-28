#include "MQTTHandler.h"
#include "TemperatureControl.h"
#include <Nextion.h>

extern LogHandler _logger;

MQTTHandler::MQTTHandler(WiFiClient& net, PubSubClient& client, SystemStatus& systemStatus, LogHandler& logger)
    : net(net), client(client), systemStatus(systemStatus), _logger(logger) {
    // Construtor
}

void MQTTHandler::connectMQTT() {
    // Configura o servidor MQTT e porta
    client.setServer(systemStatus.mqttServer, systemStatus.mqttPort);

    _logger.logMessage("Connecting to MQTT Broker...");

    while (!client.connected()) {
        // Tenta conectar com o usuário e senha
        if (client.connect("ESPDeviceClient", systemStatus.mqttUser, systemStatus.mqttPassword)) {
            _logger.logMessage("Connected to MQTT Broker");

            client.subscribe("sensor/bbq_set_temperature/set");
            client.subscribe("sensor/protein_temperature/set");
            client.subscribe("func/reset_cmd");

            // Registra a função de callback para mensagens MQTT
            client.setCallback([this](char* topic, byte* payload, unsigned int length) { this->messageHandler(topic, payload, length); });
        } else {
            _logger.logMessage("Failed to connect to MQTT Broker, rc=" + String(client.state()));
            delay(5000); // Aguarda 5 segundos antes de tentar novamente
        }
    }
}

void MQTTHandler::messageHandler(char* topic, byte* payload, unsigned int length) {
    String receivedTopic = String(topic);
    String messageTemp = String((char*)payload, length);
    _logger.logMessage("Received [" + receivedTopic + "]: " + messageTemp);

    // Verifica se a mensagem é para setar a temperatura da BBQ
    if (receivedTopic == "sensor/bbq_set_temperature/set") {
        float bbqTempSet = messageTemp.toFloat();
        if (bbqTempSet >= 30 && bbqTempSet <= 250) {
            systemStatus.bbqTemperature = bbqTempSet;
            _logger.logMessage("Nova temperatura da BBQ setada: " + String(systemStatus.bbqTemperature));
        } else {
            _logger.logMessage("Valor de BBQTempSet inválido!");
        }
    }
    // Verifica se a mensagem é para setar a temperatura da proteína
    else if (receivedTopic == "sensor/protein_temperature/set") {
        float proteinTempSet = messageTemp.toFloat();
        if (proteinTempSet >= 25 && proteinTempSet <= 100) {
            systemStatus.proteinTemperature = proteinTempSet;
            _logger.logMessage("Nova temperatura da proteína setada: " + String(systemStatus.proteinTemperature));
        } else {
            _logger.logMessage("Valor de ProteinTempSet inválido!");
        }
    }
    // Verifica se a mensagem é para resetar o sistema
    else if (receivedTopic == "func/reset_cmd") {
        // Executa a lógica de reset apenas se o payload for "reset"
        if (messageTemp == "RESET") {
            resetSystem(systemStatus);
            _logger.logMessage("Sistema Resetado");
        }
    }
}

void MQTTHandler::publishAllMessages(SystemStatus& systemStatus) {
    _logger.logMessage("=======================================");

    // Publica a temperatura da BBQ
    String bbqTemp = String(systemStatus.calibratedTemp);
    client.publish("sensor/bbq_temperature", bbqTemp.c_str());
    _logger.logMessage("BBQ temperature: " + bbqTemp);

    // Publica a temperatura setada para a BBQ
    String bbqTempSet = String(systemStatus.bbqTemperature);
    client.publish("sensor/bbq_set_temperature", bbqTempSet.c_str());
    _logger.logMessage("BBQ set temperature: " + bbqTempSet);

    // Publica o estado do relé
    String relayState = systemStatus.isRelayOn ? "electric" : "off";
    client.publish("relay/state", relayState.c_str());
    _logger.logMessage("Relay state: " + relayState);

    // Publica a temperatura da proteína
    String proteinTemp = String(systemStatus.calibratedTempP);
    client.publish("sensor/protein_temperature", proteinTemp.c_str());
    _logger.logMessage("Protein temperature: " + proteinTemp);

    // Publica a temperatura setada para a proteína
    String proteinTempSet = String(systemStatus.proteinTemperature);
    client.publish("sensor/protein_set_temperature", proteinTempSet.c_str());
    _logger.logMessage("Protein set temperature: " + proteinTempSet);

    // Publica o estado do relé para proteína
    String relayStateProtein = systemStatus.proteinTemperature > 0 ? "electric" : "off";
    client.publish("relay_protein/state", relayStateProtein.c_str());
    _logger.logMessage("Relay Protein state: " + relayStateProtein);

    // Publica a média da temperatura da BBQ
    String bbqTempAvg = String(systemStatus.averageTemp);
    client.publish("sensor/bbq_average_temperature", bbqTempAvg.c_str());
    _logger.logMessage("BBQ average temperature: " + bbqTempAvg);

    _logger.logMessage("Published all messages to MQTT topics");
    _logger.logMessage("=======================================");
}

void MQTTHandler::checkAndReconnectAwsIoT() {
    if (!client.connected()) {
        connectMQTT();
    }
    client.loop();
}

void MQTTHandler::verifyAndReconnect(SystemStatus& systemStatus) {
    _logger.logMessage("Verificando disponibilidade do MQTT Broker...");
    if (systemStatus.isHAAvailable) {
        _logger.logMessage("MQTT Broker disponível");
        connectMQTT();
    } else {
        _logger.logMessage("MQTT Broker não disponível");
    }
}

void MQTTHandler::managePublishing(SystemStatus& systemStatus) {
    static unsigned long previousMillis = 0;
    const long interval = 3000; // Intervalo de 3 segundos
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        if (systemStatus.isHAAvailable) {
            publishAllMessages(systemStatus);
        }
    }

    if (systemStatus.isHAAvailable) {
        checkAndReconnectAwsIoT();
    }
}
