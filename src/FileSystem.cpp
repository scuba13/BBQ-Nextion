#include <sqlite3.h>
#include <LittleFS.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include <Nextion.h>
#include "LogHandler.h"

#include <stdio.h>
#include <stdlib.h>
#include <SPI.h>
#include <FS.h>

extern LogHandler logHandler;

const char* data = "Callback function called";

static int callback(void *data, int argc, char **argv, char **azColName) {
    int i;
    logHandler.logMessage(String((const char*)data) + ":");
    for (i = 0; i < argc; i++) {
        logHandler.logMessage(String(azColName[i]) + " = " + (argv[i] ? argv[i] : "NULL"));
    }
    return 0;
}

int db_open(const char *filename, sqlite3 **db) {
    int rc = sqlite3_open(filename, db);
    if (rc) {
        logHandler.logMessage("Can't open database: " + String(sqlite3_errmsg(*db)));
        return rc;
    } else {
        logHandler.logMessage("Opened database successfully");
    }
    return rc;
}

int db_exec(sqlite3 *db, const char *sql) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
    if (rc != SQLITE_OK) {
        logHandler.logMessage("SQL error: " + String(zErrMsg));
        sqlite3_free(zErrMsg);
    } else {
        logHandler.logMessage("Operation done successfully");
    }
    return rc;
}

void FileSystem::initializeAndLoadConfig(SystemStatus &status, String mac)
{
    if (!LittleFS.begin(true))
    {
        logHandler.logMessage("Falha ao montar o sistema LittleFS. Tentando formatar...");
        if (!LittleFS.begin(true))
        {
            logHandler.logMessage("Falha ao formatar e montar o sistema LittleFS!");
            return;
        }
        else
        {
            logHandler.logMessage("Sistema de arquivos formatado e montado com sucesso.");
        }
    }

    // Reseta o arquivo de log se ele existir
    resetLogFile();

    // Inicializa o SQLite antes de qualquer operação com o banco de dados
    sqlite3_initialize();

    initializeDatabase(status);
    loadConfigFromDatabase(status, mac);
    
    // Teste para verificar se está gravando corretamente
    delay(1000);
    saveConfigToDatabase("mqttPort", 1111, status);
}

void FileSystem::initializeDatabase(SystemStatus &status)
{
    sqlite3 *db;
    int rc;

    if (db_open("/littlefs/config.db", &db))
        return;

// Definindo a tabela com campos de tipos específicos
const char *sql = "CREATE TABLE IF NOT EXISTS Configurations ("
                  "key TEXT PRIMARY KEY," // Chave primária para identificar a configuração
                  "isHAAvailable INTEGER," // BOOLEANO representado como INTEGER (0 ou 1)
                  "mqttServer TEXT," // Texto para o servidor MQTT
                  "mqttPort INTEGER," // Inteiro para a porta MQTT
                  "mqttUser TEXT," // Texto para o usuário MQTT
                  "mqttPassword TEXT," // Texto para a senha MQTT
                  "minBBQTemp INTEGER," // Inteiro para a temperatura mínima do BBQ
                  "maxBBQTemp INTEGER," // Inteiro para a temperatura máxima do BBQ
                  "minPrtTemp INTEGER," // Inteiro para a temperatura mínima da proteína
                  "maxPrtTemp INTEGER," // Inteiro para a temperatura máxima da proteína
                  "minCaliTemp INTEGER," // Inteiro para a temperatura mínima de calibração
                  "maxCaliTemp INTEGER," // Inteiro para a temperatura máxima de calibração
                  "minCaliTempP INTEGER," // Inteiro para a temperatura mínima de calibração da proteína
                  "maxCaliTempP INTEGER," // Inteiro para a temperatura máxima de calibração da proteína
                  "aiKey TEXT," // Texto para a chave AI
                  "tip TEXT," // Texto para a dica
                  "deviceId TEXT" // Texto para o ID do dispositivo
                  ");";


    rc = db_exec(db, sql);
    if (rc != SQLITE_OK) {
        sqlite3_close(db);
        return;
    }

    const char *check_sql = "SELECT COUNT(*) FROM Configurations;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt, 0);
        if (count == 0) {
            logHandler.logMessage("Nenhuma configuração encontrada, inserindo valores padrão.");
            insertDefaultConfigValues(db, status);
        } else {
            logHandler.logMessage("Configurações existentes encontradas no banco de dados.");
        }
        sqlite3_finalize(stmt);
    } else {
        logHandler.logMessage("Erro ao verificar configurações: " + String(sqlite3_errmsg(db)));
    }

    sqlite3_close(db);
}

void FileSystem::insertDefaultConfigValues(sqlite3 *db, SystemStatus &status)
{
    saveConfigToDatabase("isHAAvailable", "false", status);
    saveConfigToDatabase("mqttServer", "homeassistant.local", status);
    saveConfigToDatabase("mqttPort", 1883, status); // Agora como inteiro
    saveConfigToDatabase("mqttUser", "mqtt-user", status);
    saveConfigToDatabase("mqttPassword", "mqtt-user", status);

    saveConfigToDatabase("minBBQTemp", 30, status); // Agora como inteiro
    saveConfigToDatabase("maxBBQTemp", 200, status); // Agora como inteiro
    saveConfigToDatabase("minPrtTemp", 40, status); // Agora como inteiro
    saveConfigToDatabase("maxPrtTemp", 99, status); // Agora como inteiro
    saveConfigToDatabase("minCaliTemp", -20, status); // Agora como inteiro
    saveConfigToDatabase("maxCaliTemp", 20, status); // Agora como inteiro
    saveConfigToDatabase("minCaliTempP", -20, status); // Agora como inteiro
    saveConfigToDatabase("maxCaliTempP", 20, status); // Agora como inteiro

    saveConfigToDatabase("aiKey", "AIzaSyDf9K8Ya3djc2PO0YMmJmADRhuYFHMrgbc", status);
    saveConfigToDatabase("tip", "Me de 1 dica e 1 receita de Churrasco americano no total de 200 palavras. Estruture o texto com Cabecalho, Dica, Cabecalho com o nome da receita, Receita.", status);
}

void FileSystem::loadConfigFromDatabase(SystemStatus &status, String mac)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (db_open("/littlefs/config.db", &db))
        return;

    const char* keys[] = {
        "isHAAvailable", "mqttServer", "mqttPort", "mqttUser", "mqttPassword",
        "minBBQTemp", "maxBBQTemp", "minPrtTemp", "maxPrtTemp",
        "minCaliTemp", "maxCaliTemp", "minCaliTempP", "maxCaliTempP",
        "aiKey", "tip", "deviceId"
    };

    for (const char* key : keys)
    {
        String sql = "SELECT value FROM Configurations WHERE key = '";
        sql += key;
        sql += "';";

        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc == SQLITE_OK)
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                String value = String((const char*)sqlite3_column_text(stmt, 0));
                logHandler.logMessage("Carregado: " + String(key) + " = " + value);
                setStatusValue(status, key, value);
            }
            sqlite3_finalize(stmt);
        }
        else
        {
            logHandler.logMessage("Erro ao carregar configuração: " + String(sqlite3_errmsg(db)));
        }
    }

    String sql = "SELECT COUNT(*) FROM Configurations WHERE key = 'deviceId';";
    int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        int count = sqlite3_column_int(stmt, 0);
        if (count == 0)
        {
            mac.replace(":", "");
            saveConfigToDatabase("deviceId", mac.c_str(), status);
            strlcpy(status.deviceId, mac.c_str(), sizeof(status.deviceId));
        }
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
}

void FileSystem::setStatusValue(SystemStatus &status, const String &key, const String &value)
{
    if (key == "isHAAvailable") status.isHAAvailable = (value == "true");
    else if (key == "mqttServer") strlcpy(status.mqttServer, value.c_str(), sizeof(status.mqttServer));
    else if (key == "mqttPort") status.mqttPort = value.toInt();
    else if (key == "mqttUser") strlcpy(status.mqttUser, value.c_str(), sizeof(status.mqttUser));
    else if (key == "mqttPassword") strlcpy(status.mqttPassword, value.c_str(), sizeof(status.mqttPassword));
    else if (key == "minBBQTemp") status.minBBQTemp = value.toInt();
    else if (key == "maxBBQTemp") status.maxBBQTemp = value.toInt();
    else if (key == "minPrtTemp") status.minPrtTemp = value.toInt();
    else if (key == "maxPrtTemp") status.maxPrtTemp = value.toInt();
    else if (key == "minCaliTemp") status.minCaliTemp = value.toInt();
    else if (key == "maxCaliTemp") status.maxCaliTemp = value.toInt();
    else if (key == "minCaliTempP") status.minCaliTempP = value.toInt();
    else if (key == "maxCaliTempP") status.maxCaliTempP = value.toInt();
    else if (key == "aiKey") strlcpy(status.aiKey, value.c_str(), sizeof(status.aiKey));
    else if (key == "tip") strlcpy(status.tip, value.c_str(), sizeof(status.tip));
    else if (key == "deviceId") strlcpy(status.deviceId, value.c_str(), sizeof(status.deviceId));
}



void FileSystem::saveConfigToDatabase(const char* key, const char* value, SystemStatus &systemStatus)
{
    sqlite3 *db;

    if (db_open("/littlefs/config.db", &db))
        return;

    String sql = "INSERT OR REPLACE INTO Configurations (key, value) VALUES ('";
    sql += key;
    sql += "', '";
    sql += value;
    sql += "');";

    int rc = db_exec(db, sql.c_str());
    if (rc == SQLITE_OK) {
        setStatusValue(systemStatus, key, value);
    }

    sqlite3_close(db);
}

void FileSystem::verifyFileSystem()
{
    logHandler.logMessage("Verificando o sistema de arquivos LittleFS...");

    File root = LittleFS.open("/", "r");
    if (!root)
    {
        logHandler.logMessage("Falha ao abrir o diretório raiz.");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        logHandler.logMessage("FILE: " + String(file.name()) + "  SIZE: " + String(file.size()));
        file = root.openNextFile();
    }
}

void FileSystem::resetLogFile() {
    if (LittleFS.exists("/log.txt")) {
        logHandler.logMessage("Arquivo de log existente encontrado. Apagando e criando um novo...");
        LittleFS.remove("/log.txt");
    }

    File logFile = LittleFS.open("/log.txt", "w");
    if (!logFile) {
        logHandler.logMessage("Falha ao criar novo arquivo de log!");
    } else {
        logHandler.logMessage("Novo arquivo de log criado com sucesso.");
        logFile.close();
    }
}
