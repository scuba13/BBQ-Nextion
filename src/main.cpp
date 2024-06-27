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
    //Serial.begin(9600);
    Serial.println("Iniciando App ..");

    // colocar chamada pagina inicial

    // Inicialização do Nextion
    dbSerial.println("Iniciando Nextion");
    initNextion(sysStat);
    dbSerial.println("Nextion Iniciado");

    // Inicialização do Energy Monitor
    dbSerial.println("Iniciando o Energy Monitor...");
    energyMonitor.setup();
    dbSerial.println("Energy Monitor iniciado");

    // Obtenção e exibição do endereço MAC
    String mac = WiFi.macAddress();
    dbSerial.println("Endereço MAC: " + mac);

    // Inicialização do sistema de arquivos
    dbSerial.println("Inicializando FileSystem");
    fileSystem.initializeAndLoadConfig(sysStat, mac);
    dbSerial.println("FileSystem inicializado");

    // Inicialização do LogHandler
    dbSerial.println("Inicializando LogHandler");
    logHandler.logMessage("Log Inicializado");
    dbSerial.println("LogHandler inicializado");

    // Conexão WiFi
    initWiFi(sysStat, logHandler);

    // Inicialização do servidor web
    dbSerial.println("Inicializando servidor web...");
    webServerControl.begin();
    dbSerial.println("Servidor Web iniciado com sucesso");
    logHandler.logMessage("Servidor Web iniciado com sucesso");

    // Configuração do pino do relé
    dbSerial.println("Iniciando Pino do Rele");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    dbSerial.println("Pino do Rele Iniciado");

    // Criação das tarefas para obter as temperaturas calibradas
    createTasks();

    // Verificação da disponibilidade do MQTT Broker
    mqttHandler.verifyAndReconnect(sysStat);

    // Verificação e validação do sistema de arquivos
    dbSerial.println("Validando arquivos...");
    logHandler.logMessage("Validando arquivos...");
    fileSystem.verifyFileSystem();
    dbSerial.println("Arquivos validados");
    logHandler.logMessage("Arquivos validados");

    dbSerial.println("App Iniciada");
}

unsigned long lastUpdateTime = 0;         // Variável para armazenar o tempo da última atualização
const unsigned long updateInterval = 1000; // Intervalo de tempo desejado (em milissegundos)

void loop()
{

    // Atualizar os valores das variáveis do Nextion com intervalo
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval)
    {
        lastUpdateTime = currentTime;
        // Loop do Nextion
        nexLoop(nex_listen_list);

        // Gerencia a publicação e verificação do MQTT
        mqttHandler.managePublishing(sysStat);
        
        // Atualiza as variáveis do Nextion
        updateNextionMonitorVariables(sysStat);
        updateNextionSetBBQVariables(sysStat);
        updateNextionSetChunkVariables(sysStat);
        updateNextionEnergyVariables(sysStat);
    }
}
