#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include <Nextion.h>

void FileSystem::initializeAndLoadConfig(SystemStatus &status, String mac)
{

    if (!LittleFS.begin(true))
    {
        dbSerial.println("Falha ao montar o sistema LittleFS. Tentando formatar...");
        if (!LittleFS.begin(true))
        {
            dbSerial.println("Falha ao formatar e montar o sistema LittleFS!");
        }
        else
        {
            dbSerial.println("Sistema de arquivos formatado e montado com sucesso.");
        }
    }

    if (!LittleFS.exists("/config.json"))
    {
        dbSerial.println("Criando config.json com valores padrão...");
        File configFile = LittleFS.open("/config.json", "w");
        if (!configFile)
        {
            dbSerial.println("Falha ao criar config.json!");
            return;
        }

        JsonDocument doc;
        doc["isHAAvailable"] = false;
        doc["mqttServer"] = "homeassistant.local";
        doc["mqttPort"] = 1883;
        doc["mqttUser"] = "mqtt-user";
        doc["mqttPassword"] = "mqtt-user";
        doc["minBBQTemp"] = 30;
        doc["maxBBQTemp"] = 200;
        doc["minPrtTemp"] = 40;
        doc["maxPrtTemp"] = 99;
        doc["minCaliTemp"] = -20;
        doc["maxCaliTemp"] = 20;
        doc["minCaliTempP"] = -20;
        doc["maxCaliTempP"] = 20;
        doc["aiKey"] = "AIzaSyDf9K8Ya3djc2PO0YMmJmADRhuYFHMrgbc";
        doc["tip"] = "Me de 1 dica e 1 receita de Churrasco americano no total de 200 palavras. Estruture o texto com Cabecalho, Dica, Cabecalho com o nome da receita, Receita."; 
        
        mac.replace(":", "");  // Remove os dois pontos do endereço MAC
        doc["deviceId"] = mac; // Adiciona o deviceId ao documento JSON

        if (serializeJson(doc, configFile) == 0)
        {
            dbSerial.println("Falha ao escrever em config.json!");
        }
        configFile.close();
    }

    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile)
    {
        dbSerial.println("Falha ao abrir config.json para leitura!");
        return;
    }

    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size + 1]);
    configFile.readBytes(buf.get(), size);
    buf[size] = '\0';

    JsonDocument doc;
    auto error = deserializeJson(doc, buf.get());

    if (error)
    {
        dbSerial.println("Falha ao deserializeJson config.json!");
        return;
    }
    dbSerial.println("Configurações sendo Carregadas.");
    status.isHAAvailable = doc["isHAAvailable"].as<bool>();
    strlcpy(status.mqttServer, doc["mqttServer"].as<const char *>(), sizeof(status.mqttServer));
    status.mqttPort = doc["mqttPort"].as<int>();
    strlcpy(status.mqttUser, doc["mqttUser"].as<const char *>(), sizeof(status.mqttUser));
    strlcpy(status.mqttPassword, doc["mqttPassword"].as<const char *>(), sizeof(status.mqttPassword));
    strlcpy(status.deviceId, doc["deviceId"].as<const char *>(), sizeof(status.deviceId));
    status.minBBQTemp = doc["minBBQTemp"].as<int>();
    status.maxBBQTemp = doc["maxBBQTemp"].as<int>();
    status.minPrtTemp = doc["minPrtTemp"].as<int>();
    status.maxPrtTemp = doc["maxPrtTemp"].as<int>();
    status.minCaliTemp = doc["minCaliTemp"].as<int>();
    status.maxCaliTemp = doc["maxCaliTemp"].as<int>();
    status.minCaliTempP = doc["minCaliTempP"].as<int>();
    status.maxCaliTempP = doc["maxCaliTempP"].as<int>();
    strlcpy(status.aiKey, doc["aiKey"].as<const char *>(), sizeof(status.aiKey));
    strlcpy(status.tip, doc["tip"].as<const char *>(), sizeof(status.tip));

    dbSerial.print("isHAAvailable: ");
    dbSerial.println(status.isHAAvailable ? "true" : "false");
    dbSerial.print("mqttServer: ");
    dbSerial.println(status.mqttServer);
    dbSerial.print("mqttPort: ");
    dbSerial.println(status.mqttPort);
    dbSerial.print("mqttUser: ");
    dbSerial.println(status.mqttUser);
    dbSerial.print("mqttPassword: ");
    dbSerial.println(status.mqttPassword);
    dbSerial.print("deviceId: ");
    dbSerial.println(status.deviceId);
    dbSerial.print("minBBQTemp: ");
    dbSerial.println(status.minBBQTemp);
    dbSerial.print("maxBBQTemp: ");
    dbSerial.println(status.maxBBQTemp);
    dbSerial.print("minPrtTemp: ");
    dbSerial.println(status.minPrtTemp);
    dbSerial.print("maxPrtTemp: ");
    dbSerial.println(status.maxPrtTemp);
    dbSerial.print("minCaliTemp: ");
    dbSerial.println(status.minCaliTemp);
    dbSerial.print("maxCaliTemp: ");
    dbSerial.println(status.maxCaliTemp);
    dbSerial.print("minCaliTempP: ");
    dbSerial.println(status.minCaliTempP);
    dbSerial.print("maxCaliTempP: ");
    dbSerial.println(status.maxCaliTempP);
    dbSerial.print("aiKey: ");
    dbSerial.println(status.aiKey);
    dbSerial.print("tip: ");
    dbSerial.println(status.tip);

    dbSerial.println("Configurações carregadas com sucesso.");
}

void FileSystem::saveConfigToFile(const SystemStatus &status)
{

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile)
    {
        dbSerial.println("Failed to open config file for writing");
        return;
    }

    JsonDocument doc;
    doc["isHAAvailable"] = status.isHAAvailable;
    doc["mqttServer"] = status.mqttServer;
    doc["mqttPort"] = status.mqttPort;
    doc["mqttUser"] = status.mqttUser;
    doc["mqttPassword"] = status.mqttPassword;

    doc["minBBQTemp"] = status.minBBQTemp;
    doc["maxBBQTemp"] = status.maxBBQTemp;
    doc["minPrtTemp"] = status.minPrtTemp;
    doc["maxPrtTemp"] = status.maxPrtTemp;
    doc["minCaliTemp"] = status.minCaliTemp;
    doc["maxCaliTemp"] = status.maxCaliTemp;
    doc["minCaliTempP"] = status.minCaliTempP;
    doc["maxCaliTempP"] = status.maxCaliTempP;
    
    doc["aiKey"] = status.aiKey;
    doc["tip"] = status.tip;

    if (serializeJson(doc, configFile) == 0)
    {
        dbSerial.println("Failed to write to config file");
    }
    else
    {
        dbSerial.println("Configuration saved successfully");
    }

    configFile.close();
}

void FileSystem::verifyFileSystem()
{
    dbSerial.println("Verificando o sistema de arquivos LittleFS...");

    // Abre o diretório raiz no LittleFS
    File root = LittleFS.open("/", "r");
    if (!root)
    {
        dbSerial.println("Falha ao abrir o diretório raiz.");
        return;
    }

    // LittleFS não suporta diretórios, então a checagem de diretório é desnecessária
    // e sempre retornará false. Removemos a checagem de diretório.

    // Enumera todos os arquivos no diretório (raiz, neste caso)
    File file = root.openNextFile();
    while (file)
    {
        // Como LittleFS não suporta diretórios, não precisamos verificar se é um diretório
        dbSerial.print("FILE: " + String(file.name()) + "  SIZE: ");
        dbSerial.println(file.size());
        file = root.openNextFile(); // Vai para o próximo arquivo
    }
}
