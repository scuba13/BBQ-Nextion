#include "WiFiHandler.h"
#include <Arduino.h>
#include "NextionHandler.h"
#include <Nextion.h>

// Objeto WiFiManager
WiFiManager wifiManager;

void initWiFi(SystemStatus &sysStat, LogHandler &logHandler)
{
    wifi.show();
    bool isConnected = wifiManager.autoConnect("LazyQ Inc.");
    if (isConnected)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            dbSerial.println("Conexão WiFi falhou, não há credenciais salvas.");
            logHandler.logMessage("Conexão WiFi falhou, não há credenciais salvas.");
        }
        else
        {
            dbSerial.println("Conexão WiFi estabelecida!");
            logHandler.logMessage("Conexão WiFi estabelecida!");
            dbSerial.println("O IP da ESP32 é: " + WiFi.localIP().toString());
            logHandler.logMessage("O IP da ESP32 é: " + WiFi.localIP().toString());

            // Inicialização do MDNS
            if (!MDNS.begin("bbq"))
            {
                dbSerial.println("Erro ao configurar o MDNS");
                logHandler.logMessage("Erro ao configurar o MDNS");
                while (1)
                {
                    delay(1000);
                }
            }
            dbSerial.println("MDNS configurado com sucesso");
            logHandler.logMessage("MDNS configurado com sucesso");
            MDNS.addService("http", "tcp", 80);
            delay(1000);
            welcome.show();
        }
    }
    else
    {
        dbSerial.println("Falha na tentativa de autoConectar, entrando no modo AP.");
        logHandler.logMessage("Falha na tentativa de autoConectar, entrando no modo AP.");
    }
}
