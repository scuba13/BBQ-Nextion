#include <sqlite3.h>
#include <LittleFS.h>
#include "SystemStatus.h"
#include "FileSystem.h"
#include <Nextion.h>
#include "LogHandler.h"

extern LogHandler logHandler;

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

    initializeDatabase(status);
    loadConfigFromDatabase(status, mac);
}

void FileSystem::initializeDatabase(SystemStatus &status)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("/littlefs/config.db", &db);
    if (rc)
    {
        logHandler.logMessage("Não foi possível abrir o banco de dados: " + String(sqlite3_errmsg(db)));
        return;
    }
    else
    {
        logHandler.logMessage("Banco de dados aberto com sucesso");
    }

    // Criação da tabela de configurações
    const char *sql = "CREATE TABLE IF NOT EXISTS Configurations (" \
                      "key TEXT PRIMARY KEY," \
                      "value TEXT NOT NULL);";

    rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        logHandler.logMessage("Erro ao criar tabela de configurações: " + String(zErrMsg));
        sqlite3_free(zErrMsg);
    }
    else
    {
        logHandler.logMessage("Tabela de configurações criada com sucesso");
    }

    // Verifica se há registros na tabela, caso contrário, insere os valores padrão
    const char *check_sql = "SELECT COUNT(*) FROM Configurations;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        int count = sqlite3_column_int(stmt, 0);
        if (count == 0)
        {
            logHandler.logMessage("Nenhuma configuração encontrada, inserindo valores padrão.");
            insertDefaultConfigValues(db, status);
        }
        else
        {
            logHandler.logMessage("Configurações existentes encontradas no banco de dados.");
        }
        sqlite3_finalize(stmt);
    }
    else
    {
        logHandler.logMessage("Erro ao verificar configurações: " + String(sqlite3_errmsg(db)));
    }

    sqlite3_close(db);
}

void FileSystem::insertDefaultConfigValues(sqlite3 *db, SystemStatus &status)
{
    saveConfigToDatabase("isHAAvailable", "false", status);
    saveConfigToDatabase("mqttServer", "homeassistant.local", status);
    saveConfigToDatabase("mqttPort", "1883", status);
    saveConfigToDatabase("mqttUser", "mqtt-user", status);
    saveConfigToDatabase("mqttPassword", "mqtt-user", status);

    saveConfigToDatabase("minBBQTemp", "30", status);
    saveConfigToDatabase("maxBBQTemp", "200", status);
    saveConfigToDatabase("minPrtTemp", "40", status);
    saveConfigToDatabase("maxPrtTemp", "99", status);
    saveConfigToDatabase("minCaliTemp", "-20", status);
    saveConfigToDatabase("maxCaliTemp", "20", status);
    saveConfigToDatabase("minCaliTempP", "-20", status);
    saveConfigToDatabase("maxCaliTempP", "20", status);

    saveConfigToDatabase("aiKey", "AIzaSyDf9K8Ya3djc2PO0YMmJmADRhuYFHMrgbc", status);
    saveConfigToDatabase("tip", "Me de 1 dica e 1 receita de Churrasco americano no total de 200 palavras. Estruture o texto com Cabecalho, Dica, Cabecalho com o nome da receita, Receita.", status);
}

void FileSystem::loadConfigFromDatabase(SystemStatus &status, String mac)
{
    sqlite3 *db;
    sqlite3_stmt *stmt;

    int rc = sqlite3_open("/littlefs/config.db", &db);
    if (rc)
    {
        logHandler.logMessage("Não foi possível abrir o banco de dados: " + String(sqlite3_errmsg(db)));
        return;
    }

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

        rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
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

    // Se a configuração "deviceId" não estiver presente, vamos adicionar o MAC ao banco de dados
    String sql = "SELECT COUNT(*) FROM Configurations WHERE key = 'deviceId';";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
    {
        int count = sqlite3_column_int(stmt, 0);
        if (count == 0)
        {
            mac.replace(":", "");  // Remove os dois pontos do endereço MAC
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
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open("/littlefs/config.db", &db);
    if (rc)
    {
        logHandler.logMessage("Não foi possível abrir o banco de dados: " + String(sqlite3_errmsg(db)));
        return;
    }

    String sql = "INSERT OR REPLACE INTO Configurations (key, value) VALUES ('";
    sql += key;
    sql += "', '";
    sql += value;
    sql += "');";

    rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
        logHandler.logMessage("Erro ao salvar configuração: " + String(zErrMsg));
        sqlite3_free(zErrMsg);
    }
    else
    {
        logHandler.logMessage("Configuração salva com sucesso: " + String(key) + " = " + String(value));
        setStatusValue(systemStatus, key, value); // Atualiza o SystemStatus aqui
    }

    sqlite3_close(db);
}

void FileSystem::verifyFileSystem()
{
    logHandler.logMessage("Verificando o sistema de arquivos LittleFS...");

    // Abre o diretório raiz no LittleFS
    File root = LittleFS.open("/", "r");
    if (!root)
    {
        logHandler.logMessage("Falha ao abrir o diretório raiz.");
        return;
    }

    // Enumera todos os arquivos no diretório (raiz, neste caso)
    File file = root.openNextFile();
    while (file)
    {
        logHandler.logMessage("FILE: " + String(file.name()) + "  SIZE: " + String(file.size()));
        file = root.openNextFile(); // Vai para o próximo arquivo
    }
}

void FileSystem::resetLogFile() {
    if (LittleFS.exists("/log.txt")) {
        logHandler.logMessage("Arquivo de log existente encontrado. Apagando e criando um novo...");
        LittleFS.remove("/log.txt");
    }

    // Cria um novo arquivo de log vazio
    File logFile = LittleFS.open("/log.txt", "w");
    if (!logFile) {
        logHandler.logMessage("Falha ao criar novo arquivo de log!");
    } else {
        logHandler.logMessage("Novo arquivo de log criado com sucesso.");
        logFile.close();
    }
}
