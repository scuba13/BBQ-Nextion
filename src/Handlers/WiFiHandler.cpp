#include "WiFiHandler.h"
#include <Arduino.h>
#include "NextionHandler.h"
#include "LogHandler.h"

extern LogHandler _logger;

// Configurações otimizadas de WiFi
#define WIFI_CONNECT_TIMEOUT 10000  // 10 segundos timeout
#define WIFI_RETRY_DELAY 500        // Delay entre tentativas
#define MAX_CONNECTION_RETRIES 3    // Máximo de tentativas

void configModeCallback(WiFiManager *myWiFiManager) {
    _logger.logMessage("Modo AP iniciado: " + myWiFiManager->getConfigPortalSSID());
    digitalWrite(RGB_BUILTIN, HIGH);
    ap.show();
}

void initWiFi(SystemStatus &sysStat, LogHandler &logHandler) {
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    
    wifi.show();
    
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setConfigPortalTimeout(120);
    
    if (wifiManager.autoConnect("LazyQ Inc.")) {
        logHandler.logMessage("WiFi conectado - IP: " + WiFi.localIP().toString());
        
        if (MDNS.begin("bbq")) {
            MDNS.addService("http", "tcp", 80);
        }
        
        welcome.show();
    } else {
        logHandler.logMessage("Falha na conexão WiFi");
    }
}
