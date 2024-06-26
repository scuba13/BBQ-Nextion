#include <WiFiManager.h>
#include "SystemStatus.h"
#include "PinDefinitions.h"
#include "WebServerControl.h"
#include "TemperatureControl.h"
#include <Arduino.h>
#include <PubSubClient.h>
#include "MQTTHandler.h"
#include <ESPmDNS.h>
#include "LogHandler.h"
#include "FileSystem.h"
#include <ESPAsyncWebServer.h>
#include "EnergyMonitor.h"
#include "NextionHandler.h"
#include "TaskHandler.h"
#include "WiFiHandler.h"
#include "/home/edu/.platformio/packages/framework-arduinoespressif32/libraries/SD/src/SD.h"

// Instanciação dos objetos das classes
SystemStatus sysStat;
FileSystem fileSystem;
WiFiClient net = WiFiClient();
PubSubClient client(net);
LogHandler logHandler;
AsyncWebServer server(80);
WebServerControl webServerControl(sysStat, fileSystem, logHandler, server);
MQTTHandler mqttHandler(net, client, sysStat, logHandler);
EnergyMonitor energyMonitor(sysStat);

void setup()
{
    Serial.begin(9600);
    Serial.println("Iniciando App ..");

    //colocar chamada pagina inicial

    // Inicialização do Nextion
    Serial.println("Iniciando Nextion");
    initNextion(sysStat);
    Serial.println("Nextion Iniciado");

    // Inicialização do Energy Monitor
    Serial.println("Iniciando o Energy Monitor...");
    energyMonitor.setup();
    Serial.println("Energy Monitor iniciado");

    // Obtenção e exibição do endereço MAC
    String mac = WiFi.macAddress();
    Serial.println("Endereço MAC: " + mac);

    // Inicialização do sistema de arquivos
    Serial.println("Inicializando FileSystem");
    fileSystem.initializeAndLoadConfig(sysStat, mac);
    Serial.println("FileSystem inicializado");

    // Inicialização do LogHandler
    Serial.println("Inicializando LogHandler");
    logHandler.logMessage("Log Inicializado");
    Serial.println("LogHandler inicializado");

    // Conexão WiFi
    initWiFi(sysStat, logHandler);

    // Inicialização do servidor web
    Serial.println("Inicializando servidor web...");
    webServerControl.begin();
    Serial.println("Servidor Web iniciado com sucesso");
    logHandler.logMessage("Servidor Web iniciado com sucesso");

    // Configuração do pino do relé
    Serial.println("Iniciando Pino do Rele");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Pino do Rele Iniciado");

    // Criação das tarefas para obter as temperaturas calibradas
    createTasks();

    // Verificação da disponibilidade do MQTT Broker
    mqttHandler.verifyAndReconnect(sysStat);

    // Verificação e validação do sistema de arquivos
    Serial.println("Validando arquivos...");
    logHandler.logMessage("Validando arquivos...");
    fileSystem.verifyFileSystem();
    Serial.println("Arquivos validados");
    logHandler.logMessage("Arquivos validados");

    Serial.println("App Iniciada");
}

void loop()
{

    // Loop do Nextion
    nexLoop(nex_listen_list);



    // Gerencia a publicação e verificação do MQTT
    mqttHandler.managePublishing(sysStat);

    // Atualizar os valores das variáveis do Nextion
    updateNextionMonitorVariables(sysStat);
    updateNextionSetBBQVariables(sysStat);
    updateNextionSetChunkVariables(sysStat);
    updateNextionEnergyVariables(sysStat);
    delay(500);
}
