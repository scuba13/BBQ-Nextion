#include "WiFiHandler.h"
#include <Arduino.h>
#include "NextionHandler.h"
#include "AppContext.h"
#include <WiFiManager.h>

// Constantes
constexpr uint32_t WIFI_CONNECT_TIMEOUT = 180;  // 3 minutos
constexpr uint32_t WIFI_CONFIG_PORTAL_TIMEOUT = 120;  // 2 minutos
constexpr uint32_t LED_BLINK_INTERVAL = 500;  // 0.5 segundos

// Cache de estado do WiFi
static struct {
    bool isConfigMode = false;
    unsigned long lastBlink = 0;
    bool ledState = false;
} wifiState;

// Configuração do WiFiManager
static WiFiManager wifiManager;

// Callback otimizado para modo AP
void configModeCallback(WiFiManager *myWiFiManager) {
    if (!wifiState.isConfigMode) {
        app.logHandler.logMessage("Modo AP iniciado: " + myWiFiManager->getConfigPortalSSID());
        app.logHandler.logMessage("IP: " + WiFi.softAPIP().toString());
        wifiState.isConfigMode = true;
        ap.show();  // Mostrar tela AP apenas uma vez
    }
}

// Callback para quando o WiFi conecta
void wifiConnectedCallback() {
    app.logHandler.logMessage("WiFi conectado: " + WiFi.SSID());
    app.logHandler.logMessage("IP: " + WiFi.localIP().toString());
    welcome.show();
}

// Configurar MDNS de forma mais robusta
bool setupMDNS() {
    if (!MDNS.begin("bbq")) {
        app.logHandler.logMessage("Erro ao configurar MDNS");
        return false;
    }
    
    MDNS.addService("http", "tcp", 80);
    app.logHandler.logMessage("MDNS configurado: bbq.local");
    return true;
}

void initWiFi(SystemStatus &sysStat, LogHandler &logger) {
    app.logHandler.logMessage("Iniciando WiFi...");

    // Configurar WiFiManager
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setConnectTimeout(WIFI_CONNECT_TIMEOUT);
    wifiManager.setConfigPortalTimeout(WIFI_CONFIG_PORTAL_TIMEOUT);
    
    // Configurações adicionais
    wifiManager.setBreakAfterConfig(true);
    wifiManager.setDebugOutput(false);  // Desabilitar debug para melhor performance
    
    // Tentar conectar
    if (!wifiManager.autoConnect("BBQ-Controller")) {
        app.logHandler.logMessage("Falha na conexão - reiniciando...");
        delay(1000);
        ESP.restart();
        return;
    }

    // Verificar conexão
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnectedCallback();
        setupMDNS();
    }
}

// Função para verificar e reconectar WiFi se necessário
void checkWiFiConnection() {
    static unsigned long lastCheck = 0;
    const unsigned long CHECK_INTERVAL = 30000;  // Verificar a cada 30 segundos
    
    unsigned long now = millis();
    if (now - lastCheck < CHECK_INTERVAL) {
        return;
    }
    lastCheck = now;
    
    if (WiFi.status() != WL_CONNECTED) {
        app.logHandler.logMessage("Reconectando WiFi...");
        WiFi.reconnect();
    }
}

// Função para atualizar LED de status
void updateWiFiStatusLED() {
    if (!wifiState.isConfigMode) {
        return;
    }
    
    unsigned long now = millis();
    if (now - wifiState.lastBlink >= LED_BLINK_INTERVAL) {
        wifiState.ledState = !wifiState.ledState;
        digitalWrite(RGB_BUILTIN, wifiState.ledState ? HIGH : LOW);
        wifiState.lastBlink = now;
    }
}
