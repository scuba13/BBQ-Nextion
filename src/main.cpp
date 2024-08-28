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
#include "NextionHandler.h"
#include "TaskHandler.h"
#include "WiFiHandler.h"

// Instanciação dos objetos das classes
SystemStatus sysStat;
FileSystem fileSystem;
WiFiClient net = WiFiClient();
PubSubClient client(net);
LogHandler logHandler;
AsyncWebServer server(80);
WebServerControl webServerControl(sysStat, fileSystem, logHandler, server);
MQTTHandler mqttHandler(net, client, sysStat, logHandler);
LogHandler _logger; 

void setup()
{
    logHandler.logMessage("Iniciando App ..");

    // Inicialização do Nextion
    logHandler.logMessage("Iniciando Nextion");
    initNextion(sysStat);
    logHandler.logMessage("Nextion Iniciado");

    // Obtenção e exibição do endereço MAC
    String mac = WiFi.macAddress();
    logHandler.logMessage("Endereço MAC: " + mac);

    // Inicialização do sistema de arquivos
    logHandler.logMessage("Inicializando FileSystem");
    fileSystem.initializeAndLoadConfig(sysStat, mac);
    logHandler.logMessage("FileSystem inicializado");

    // Inicialização do LogHandler
    logHandler.logMessage("Inicializando LogHandler");
    logHandler.logMessage("Log Inicializado");
    logHandler.logMessage("LogHandler inicializado");

    // Configuração do pino do relé
    logHandler.logMessage("Iniciando Pino do Rele");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    logHandler.logMessage("Pino do Rele Iniciado");

    // Criação das tarefas para obter as temperaturas calibradas
    logHandler.logMessage("Criando tarefas...");
    createTasks();
    logHandler.logMessage("Tarefas criadas");

    // Verificação da disponibilidade do MQTT Broker
    mqttHandler.verifyAndReconnect(sysStat);

    // Verificação e validação do sistema de arquivos
    logHandler.logMessage("Validando arquivos...");
    fileSystem.verifyFileSystem();
    logHandler.logMessage("Arquivos validados");

    // Conexão WiFi
    logHandler.logMessage("Iniciando WiFi");
    initWiFi(sysStat, logHandler);
    logHandler.logMessage("WiFi Iniciado");

    // Inicialização do servidor web
    logHandler.logMessage("Inicializando servidor web...");
    webServerControl.begin();
    logHandler.logMessage("Servidor Web iniciado com sucesso");

    // Verifique se a PSRAM está disponível
    if (psramFound())
    {
        logHandler.logMessage("PSRAM found and initialized.");
    }
    else
    {
        logHandler.logMessage("PSRAM not found.");
    }

    // Verifique o tamanho da PSRAM
    logHandler.logMessage("PSRAM size: " + String(ESP.getPsramSize() / 1024) + " KB");

    // Verifique a memória PSRAM livre
    logHandler.logMessage("Free PSRAM: " + String(ESP.getFreePsram() / 1024) + " KB");

    // Verifique o tamanho total da RAM interna
    logHandler.logMessage("Total heap size: " + String(ESP.getHeapSize() / 1024) + " KB");

    // Verifique a memória RAM interna livre
    logHandler.logMessage("Free heap size: " + String(ESP.getFreeHeap() / 1024) + " KB");

    // Verifique o tamanho total da DRAM interna
    logHandler.logMessage("Total DRAM: " + String(ESP.getMaxAllocHeap() / 1024) + " KB");

    // Verifique o tamanho total da memória de código
    logHandler.logMessage("Total instruction RAM: " + String(ESP.getFlashChipSize() / 1024) + " KB");

    logHandler.logMessage("App Iniciada");
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green
    delay(1000);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off / black
}

unsigned long lastUpdateTime = 0;         // Variável para armazenar o tempo da última atualização
const unsigned long updateInterval = 500; // Intervalo de tempo desejado (em milissegundos)

void loop()
{
    // Atualizar os valores das variáveis do Nextion com intervalo
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= updateInterval)
    {
        lastUpdateTime = currentTime;
        // Loop do Nextion
        nexLoop(nex_listen_list);
        updateNextionMonitorVariables(sysStat);
        updateNextionSetBBQVariables(sysStat);
        updateNextionSetChunkVariables(sysStat);
        updateNextionSetCaliVariables(sysStat);

        // Gerencia a publicação e verificação do MQTT
        mqttHandler.managePublishing(sysStat);
    }
}
