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
#include <functional>
#include "AppContext.h"

// Definição global da instância de AppContext
AppContext app;

// Declaração antecipada
void updateNextionVariables(SystemStatus& sysStat);

void setup() {
    // Estrutura de inicialização mais robusta
    struct InitStep {
        const char* name;
        std::function<bool()> func;
    };

    const InitStep initSteps[] = {
        {"Nextion", []() -> bool {
            initNextion(app.sysStat);
            initial.show();
            return true;
        }},
        {"FileSystem", []() -> bool {
            String mac = WiFi.macAddress();
            app.fileSystem.initializeAndLoadConfig(app.sysStat, mac);
            return true;
        }},
        {"Relay", []() -> bool {
            pinMode(RELAY_PIN, OUTPUT);
            digitalWrite(RELAY_PIN, LOW);
            return true;
        }},
        {"Tasks", []() -> bool {
            createTasks();
            return true;
        }},
        {"WiFi", []() -> bool {
            initWiFi(app.sysStat, app.logHandler);
            return true;
        }},
        {"WebServer", []() -> bool {
            app.webServerControl.begin();
            return true;
        }}
    };

    // LED de status
    neopixelWrite(RGB_BUILTIN, 0, 0, RGB_BRIGHTNESS); // Blue - iniciando

    // Inicialização sequencial com tratamento de erros
    for (const auto& step : initSteps) {
        app.logHandler.logMessage("Iniciando " + String(step.name));
        if (!step.func()) {
            app.logHandler.logError("Falha ao iniciar " + String(step.name));
            neopixelWrite(RGB_BUILTIN, RGB_BRIGHTNESS, 0, 0); // Red - erro
            delay(1000);
            ESP.restart();
        }
        app.logHandler.logMessage(String(step.name) + " iniciado");
    }

    // Verificações de memória apenas em DEBUG
    #ifdef DEBUG_MEMORY
        logMemoryStatus();
    #endif

    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0); // Green - sucesso
    delay(1000);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0); // Off
}

// Função auxiliar para log de memória
#ifdef DEBUG_MEMORY
void logMemoryStatus() {
    const char* memoryLabels[] = {
        "PSRAM", "Free PSRAM", "Total heap", "Free heap",
        "Total DRAM", "Total instruction RAM"
    };
    const uint32_t memoryValues[] = {
        ESP.getPsramSize(), ESP.getFreePsram(),
        ESP.getHeapSize(), ESP.getFreeHeap(),
        ESP.getMaxAllocHeap(), ESP.getFlashChipSize()
    };

    for (int i = 0; i < 6; i++) {
        app.logHandler.logMessage(String(memoryLabels[i]) + ": " + 
            String(memoryValues[i] / 1024) + " KB");
    }
}
#endif

// Mover a definição da função para antes do loop
void updateNextionVariables(SystemStatus& sysStat) {
    updateNextionMonitorVariables(sysStat);
    updateNextionSetBBQVariables(sysStat);
    updateNextionSetChunkVariables(sysStat);
    updateNextionSetCaliVariables(sysStat);
}

void loop() {
    static unsigned long lastUpdate = 0;
    const unsigned long UPDATE_INTERVAL = 1000;
    
    // Processar Nextion continuamente
    nexLoop(nex_listen_list);
    
    // Processar WebServer e MQTT
    app.webServerControl.loop();
    
    if (WiFi.status() == WL_CONNECTED && app.sysStat.isHAAvailable) {
        app.mqttHandler.loop();
    }
    
    // Atualizações periódicas
    unsigned long now = millis();
    if (now - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = now;
        
        // Atualizar display Nextion
        updateNextionVariables(app.sysStat);
        
        // Outras atualizações periódicas aqui
    }
}
