#include "Handlers/WiFiHandler.h"
#include <Arduino.h>
#include <time.h>
#include "Handlers/NextionHandler.h"
#include "Handlers/LogHandler.h"

extern LogHandler logHandler;

// Configurações otimizadas de WiFi
#define WIFI_CONNECT_TIMEOUT 10000  // 10 segundos timeout
#define WIFI_RETRY_DELAY 500        // Delay entre tentativas
#define MAX_CONNECTION_RETRIES 3    // Máximo de tentativas

void configModeCallback(WiFiManager *myWiFiManager) {
    logHandler.logMessage("Modo AP iniciado: " + myWiFiManager->getConfigPortalSSID());
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

        // Sincroniza hora via NTP (UTC-3 = Brasília)
        configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
        logHandler.logMessage("NTP sincronizando...");

        if (MDNS.begin("bbq")) {
            MDNS.addService("http", "tcp", 80);
        }

        welcome.show();
    } else {
        logHandler.logMessage("Falha na conexão WiFi");
    }
}
