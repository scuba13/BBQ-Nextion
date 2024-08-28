#include "WiFiHandler.h"
#include <Arduino.h>
#include "NextionHandler.h"
#include <Nextion.h>
#include "LogHandler.h"
extern LogHandler _logger;
// Objeto WiFiManager
WiFiManager wifiManager;

void initWiFi(SystemStatus &sysStat, LogHandler &logHandler)
{
    logHandler.logMessage("Iniciando processo de conexão WiFi...");
    wifi.show();
    
    bool isConnected = wifiManager.autoConnect("LazyQ Inc.");
    
    if (isConnected)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            logHandler.logMessage("Conexão WiFi falhou, não há credenciais salvas.");
        }
        else
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
    }
    else
    {
        logHandler.logMessage("Falha na tentativa de autoConectar, entrando no modo AP.");
    }
}
