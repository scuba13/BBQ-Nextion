#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#include "SystemStatus.h"
#include "Handlers/FileSystem.h"
#include "Handlers/LogHandler.h"

extern LogHandler logHandler;

FileSystem::FileSystem() {}

// Aplica os valores padrão ao SystemStatus quando não há config válida
static void applyDefaults(SystemStatus &status) {
    status.bbqTemperature     = 0;
    status.proteinTemperature = 0;
    status.tempCalibration    = 0;
    status.tempCalibrationP   = 0;
    status.isHAAvailable      = false;
    status.mqttPort           = 1883;
    status.minBBQTemp         = DEFAULT_MIN_BBQ_TEMP;
    status.maxBBQTemp         = DEFAULT_MAX_BBQ_TEMP;
    status.minPrtTemp         = DEFAULT_MIN_PRT_TEMP;
    status.maxPrtTemp         = DEFAULT_MAX_PRT_TEMP;
    status.minCaliTemp        = DEFAULT_MIN_CALI_TEMP;
    status.maxCaliTemp        = DEFAULT_MAX_CALI_TEMP;
    status.minCaliTempP       = DEFAULT_MIN_CALI_TEMP;
    status.maxCaliTempP       = DEFAULT_MAX_CALI_TEMP;
}

static bool loadFromJson(const JsonDocument &doc, SystemStatus &status) {
    status.isHAAvailable      = doc["isHAAvailable"]  | false;
    status.mqttPort           = doc["mqttPort"]        | 1883;
    status.bbqTemperature     = doc["bbqTemperature"]  | 0;
    status.proteinTemperature = doc["proteinTemperature"] | 0;
    status.tempCalibration    = doc["tempCalibration"] | 0;
    status.tempCalibrationP   = doc["tempCalibrationP"]| 0;
    status.minBBQTemp         = doc["minBBQTemp"]      | DEFAULT_MIN_BBQ_TEMP;
    status.maxBBQTemp         = doc["maxBBQTemp"]      | DEFAULT_MAX_BBQ_TEMP;
    status.minPrtTemp         = doc["minPrtTemp"]      | DEFAULT_MIN_PRT_TEMP;
    status.maxPrtTemp         = doc["maxPrtTemp"]      | DEFAULT_MAX_PRT_TEMP;
    status.minCaliTemp        = doc["minCaliTemp"]     | DEFAULT_MIN_CALI_TEMP;
    status.maxCaliTemp        = doc["maxCaliTemp"]     | DEFAULT_MAX_CALI_TEMP;
    status.minCaliTempP       = doc["minCaliTempP"]    | DEFAULT_MIN_CALI_TEMP;
    status.maxCaliTempP       = doc["maxCaliTempP"]    | DEFAULT_MAX_CALI_TEMP;

    strlcpy(status.mqttServer,  doc["mqttServer"]  | "homeassistant.local", sizeof(status.mqttServer));
    strlcpy(status.mqttUser,    doc["mqttUser"]    | "mqtt-user",           sizeof(status.mqttUser));
    strlcpy(status.mqttPassword,doc["mqttPassword"]| "mqtt-user",           sizeof(status.mqttPassword));
    strlcpy(status.deviceId,    doc["deviceId"]    | "",                    sizeof(status.deviceId));
    strlcpy(status.aiKey,       doc["aiKey"]       | "",                    sizeof(status.aiKey));
    strlcpy(status.tip,         doc["tip"]         | "",                    sizeof(status.tip));
    strlcpy(status.apiKey,      doc["apiKey"]      | "",                    sizeof(status.apiKey));
    return true;
}

// M-01: limite de tamanho antes de alocar
static const size_t MAX_CONFIG_SIZE = 8192;

static bool parseFile(File &file, SystemStatus &status) {
    size_t size = file.size();
    if (size == 0 || size > MAX_CONFIG_SIZE) {
        logHandler.logError("config.json tamanho inválido: " + String(size));
        return false;
    }

    std::unique_ptr<char[]> buf(new char[size + 1]);
    file.readBytes(buf.get(), size);
    buf[size] = '\0';

    JsonDocument doc;
    if (deserializeJson(doc, buf.get())) return false;

    return loadFromJson(doc, status);
}

void FileSystem::initializeAndLoadConfig(SystemStatus &status, String mac)
{
    if (!LittleFS.begin(true)) {
        logHandler.logMessage("Falha ao montar LittleFS — tentando formatar...");
        if (!LittleFS.begin(true)) {
            logHandler.logError("Falha ao formatar e montar LittleFS!");
            applyDefaults(status);
            return;
        }
    }

    logHandler.logMessage("=== BOOT ===");

    if (!LittleFS.exists("/config.json")) {
        logHandler.logMessage("Criando config.json com valores padrão...");
        applyDefaults(status);
        mac.replace(":", "");
        strlcpy(status.deviceId, mac.c_str(), sizeof(status.deviceId));
        strlcpy(status.mqttServer,   "homeassistant.local", sizeof(status.mqttServer));
        strlcpy(status.mqttUser,     "mqtt-user",           sizeof(status.mqttUser));
        strlcpy(status.mqttPassword, "mqtt-user",           sizeof(status.mqttPassword));
        strlcpy(status.tip, "Me de 1 dica e 1 receita de Churrasco americano no total de 200 palavras. Estruture o texto com Cabecalho, Dica, Cabecalho com o nome da receita, Receita.", sizeof(status.tip));
        status.mqttPort = 1883;
        saveConfigToFile(status);
        logHandler.logMessage("config.json criado com valores padrão.");
        return;
    }

    // Tenta carregar config.json principal
    File configFile = LittleFS.open("/config.json", "r");
    bool loaded = configFile && parseFile(configFile, status);
    if (configFile) configFile.close();

    // A-04: fallback para backup se principal falhou
    if (!loaded) {
        logHandler.logError("config.json inválido — tentando config.bak.json...");
        File bakFile = LittleFS.open("/config.bak.json", "r");
        loaded = bakFile && parseFile(bakFile, status);
        if (bakFile) bakFile.close();

        if (loaded) {
            logHandler.logMessage("Carregado do backup com sucesso.");
            saveConfigToFile(status); // restaura o principal
        } else {
            logHandler.logError("Backup também inválido — usando valores padrão.");
            applyDefaults(status);
            mac.replace(":", "");
            strlcpy(status.deviceId, mac.c_str(), sizeof(status.deviceId));
        }
    }

    logHandler.logMessage("isHAAvailable: "    + String(status.isHAAvailable ? "true" : "false"));
    logHandler.logMessage("mqttServer: "        + String(status.mqttServer));
    logHandler.logMessage("mqttPort: "          + String(status.mqttPort));
    logHandler.logMessage("deviceId: "          + String(status.deviceId));
    logHandler.logMessage("bbqTemperature: "    + String(status.bbqTemperature));
    logHandler.logMessage("proteinTemperature: "+ String(status.proteinTemperature));
    logHandler.logMessage("tempCalibration: "   + String(status.tempCalibration));
    logHandler.logMessage("tempCalibrationP: "  + String(status.tempCalibrationP));
    logHandler.logMessage("aiKey: "             + String(status.aiKey[0] ? "[configurada]" : "[vazia]"));
    logHandler.logMessage("Configurações carregadas com sucesso.");
}

bool FileSystem::saveConfigToFile(const SystemStatus &status)
{
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        logHandler.logError("Falha ao abrir config.json para escrita");
        return false;
    }

    JsonDocument doc;
    doc["isHAAvailable"]      = status.isHAAvailable;
    doc["mqttServer"]         = status.mqttServer;
    doc["mqttPort"]           = status.mqttPort;
    doc["mqttUser"]           = status.mqttUser;
    doc["mqttPassword"]       = status.mqttPassword;
    doc["deviceId"]           = status.deviceId;
    doc["bbqTemperature"]     = status.bbqTemperature;
    doc["proteinTemperature"] = status.proteinTemperature;
    doc["tempCalibration"]    = status.tempCalibration;
    doc["tempCalibrationP"]   = status.tempCalibrationP;
    doc["minBBQTemp"]         = status.minBBQTemp;
    doc["maxBBQTemp"]         = status.maxBBQTemp;
    doc["minPrtTemp"]         = status.minPrtTemp;
    doc["maxPrtTemp"]         = status.maxPrtTemp;
    doc["minCaliTemp"]        = status.minCaliTemp;
    doc["maxCaliTemp"]        = status.maxCaliTemp;
    doc["minCaliTempP"]       = status.minCaliTempP;
    doc["maxCaliTempP"]       = status.maxCaliTempP;
    doc["aiKey"]              = status.aiKey;
    doc["tip"]                = status.tip;
    doc["apiKey"]             = status.apiKey;

    bool ok = serializeJson(doc, configFile) > 0;
    configFile.close();

    if (!ok) {
        logHandler.logError("Falha ao serializar config.json");
        return false;
    }

    // Backup após save bem-sucedido
    File src = LittleFS.open("/config.json", "r");
    File bak = LittleFS.open("/config.bak.json", "w");
    if (src && bak) {
        uint8_t buf[128];
        size_t n;
        while ((n = src.read(buf, sizeof(buf))) > 0) bak.write(buf, n);
    }
    if (src) src.close();
    if (bak) bak.close();

    logHandler.logMessage("Configuração salva com sucesso");
    return true;
}

void FileSystem::verifyFileSystem()
{
    File root = LittleFS.open("/", "r");
    if (!root) { logHandler.logMessage("Falha ao abrir diretório raiz."); return; }
    File file = root.openNextFile();
    while (file) {
        logHandler.logMessage("FILE: " + String(file.name()) + "  SIZE: " + String(file.size()));
        file = root.openNextFile();
    }
}

void FileSystem::resetLogFile() {
    if (LittleFS.exists("/log.txt")) LittleFS.remove("/log.txt");
    File logFile = LittleFS.open("/log.txt", "w");
    if (!logFile) {
        logHandler.logError("Falha ao criar arquivo de log!");
    } else {
        logHandler.logMessage("Arquivo de log recriado.");
        logFile.close();
    }
}

bool FileSystem::begin() {
    if (!LittleFS.begin(true)) {
        logHandler.logMessage("Erro ao montar LittleFS");
        return false;
    }
    return true;
}

String FileSystem::readFile(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) return "";
    String content = file.readString();
    file.close();
    return content;
}

bool FileSystem::writeFile(const char* path, const char* message) {
    File file = LittleFS.open(path, "w");
    if (!file) return false;
    const size_t BUF = 256;
    size_t len = strlen(message), written = 0;
    while (written < len) {
        size_t n = min(BUF, len - written);
        if (file.write((uint8_t*)message + written, n) != n) { file.close(); return false; }
        written += n;
    }
    file.flush();
    file.close();
    return true;
}

bool FileSystem::deleteFile(const char* path) {
    return LittleFS.remove(path);
}

void FileSystem::listDir(const char* dirname) {
    File root = LittleFS.open(dirname);
    if (!root || !root.isDirectory()) { logHandler.logMessage("Falha ao abrir diretório"); return; }
    File file = root.openNextFile();
    while (file) {
        logHandler.logMessage(String(file.name()) + " - " + String(file.size()) + "B");
        file = root.openNextFile();
    }
}
