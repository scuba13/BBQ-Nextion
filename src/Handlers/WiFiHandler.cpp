#include "WiFiHandler.h"
#include <Arduino.h>
#include "NextionHandler.h"
#include <Nextion.h>
#include "LogHandler.h"
#include <WiFiManager.h>

extern LogHandler _logger;

// Objeto WiFiManager
WiFiManager wifiManager;

void configModeCallback(WiFiManager *myWiFiManager) {
    _logger.logMessage("Entrando no modo AP.");
    _logger.logMessage("SSID do AP: " + myWiFiManager->getConfigPortalSSID());
    _logger.logMessage("Endereço IP do AP: " + WiFi.softAPIP().toString());
    
    // Exibir tela informando que o dispositivo está em modo AP
   ap.show();
}

void initWiFi(SystemStatus &sysStat, LogHandler &logHandler)
{
    logHandler.logMessage("Iniciando processo de conexão WiFi...");
    wifi.show();
    
    // Set callback que será chamado quando o modo AP for iniciado
    wifiManager.setAPCallback(configModeCallback);
    
    bool isConnected = wifiManager.autoConnect("LazyQ Inc.");
    
    logHandler.logMessage("Resultado do autoConnect: " + String(isConnected ? "Conectado" : "Falha na conexão"));

    if (isConnected && WiFi.status() == WL_CONNECTED)
    {
        logHandler.logMessage("Conexão WiFi estabelecida!");
        logHandler.logMessage("O IP da ESP32 é: " + WiFi.localIP().toString());

        // Inicialização do MDNS
        if (!MDNS.begin("bbq"))
        {
            logHandler.logMessage("Erro ao configurar o MDNS");
            while (1)
            {
                delay(1000);
            }
        }
        logHandler.logMessage("MDNS configurado com sucesso");
        MDNS.addService("http", "tcp", 80);
        delay(1000);
        welcome.show();
        
    }
    else
    {
        logHandler.logMessage("Falha na tentativa de autoConectar.");
        // Exibir tela adicional, se necessário, quando a falha na conexão WiFi não resulta em modo AP
    }
}
