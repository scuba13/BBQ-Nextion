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

void setup()
{

    dbSerial.println("Iniciando App ..");

    // colocar chamada pagina inicial

    // Inicialização do Nextion
    dbSerial.println("Iniciando Nextion");
    initNextion(sysStat);
    dbSerial.println("Nextion Iniciado");

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

    // Configuração do pino do relé
    dbSerial.println("Iniciando Pino do Rele");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    dbSerial.println("Pino do Rele Iniciado");

    // Criação das tarefas para obter as temperaturas calibradas
    dbSerial.println("Criando tarefas...");
    createTasks();
    dbSerial.println("Tarefas criadas");

    // Verificação da disponibilidade do MQTT Broker
    mqttHandler.verifyAndReconnect(sysStat);

    // Verificação e validação do sistema de arquivos
    dbSerial.println("Validando arquivos...");
    logHandler.logMessage("Validando arquivos...");
    fileSystem.verifyFileSystem();
    dbSerial.println("Arquivos validados");
    logHandler.logMessage("Arquivos validados");

    // Conexão WiFi
    dbSerial.println("Iniciando WiFi");
    initWiFi(sysStat, logHandler);
    dbSerial.println("WiFi Iniciado");

    // Inicialização do servidor web
    dbSerial.println("Inicializando servidor web...");
    webServerControl.begin();
    dbSerial.println("Servidor Web iniciado com sucesso");
    logHandler.logMessage("Servidor Web iniciado com sucesso");

    // Verifique se a PSRAM está disponível
    if (psramFound())
    {
        dbSerial.println("PSRAM found and initialized.");
    }
    else
    {
        dbSerial.println("PSRAM not found.");
    }

    // Verifique o tamanho da PSRAM
    dbSerial.print("PSRAM size: ");
    dbSerial.print(ESP.getPsramSize() / 1024);
    dbSerial.println(" KB");

    // Verifique a memória PSRAM livre
    dbSerial.print("Free PSRAM: ");
    dbSerial.print(ESP.getFreePsram() / 1024);
    dbSerial.println(" KB");

    // Verifique o tamanho total da RAM interna
    dbSerial.print("Total heap size: ");
    dbSerial.print(ESP.getHeapSize() / 1024);
    dbSerial.println(" KB");

    // Verifique a memória RAM interna livre
    dbSerial.print("Free heap size: ");
    dbSerial.print(ESP.getFreeHeap() / 1024);
    dbSerial.println(" KB");

    // Verifique o tamanho total da DRAM interna
    dbSerial.print("Total DRAM: ");
    dbSerial.print(ESP.getMaxAllocHeap() / 1024);
    dbSerial.println(" KB");

    // Verifique o tamanho total da memória de código
    dbSerial.print("Total instruction RAM: ");
    dbSerial.print(ESP.getFlashChipSize() / 1024);
    dbSerial.println(" KB");

    dbSerial.println("App Iniciada");
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
        //     // Loop do Nextion
        nexLoop(nex_listen_list);
        updateNextionMonitorVariables(sysStat);
        updateNextionSetBBQVariables(sysStat);
        updateNextionSetChunkVariables(sysStat);
        updateNextionSetCaliVariables(sysStat);

        // Gerencia a publicação e verificação do MQTT
        mqttHandler.managePublishing(sysStat);
    }
}
