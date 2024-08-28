#include "LogHandler.h"
#include <FS.h>
#include <LittleFS.h>
#include <Nextion.h>

LogHandler::LogHandler() {
    // Inicializa o LittleFS se ainda não estiver inicializado
    if (!LittleFS.begin()) {
        dbSerial.println("Falha ao montar o sistema de arquivos!");
    }
}

void LogHandler::logRequest(AsyncWebServerRequest *request, const String &message) {
    String logMsg = formatLogMessage("[LOG]", request->client()->remoteIP().toString(), request->methodToString(), request->url(), message);
    logToSerial(logMsg);
    logToFile(logMsg);
}

void LogHandler::logMessage(const String &message) {
    String logMsg = "[LOG] " + message;
    logToSerial(logMsg);
    logToFile(logMsg);
}

void LogHandler::logError(const String &message) {
    String logMsg = "[ERROR] " + message;
    logToSerial(logMsg);
    logToFile(logMsg);
}

String LogHandler::formatLogMessage(const String& level, const String& clientIP, const String& method, const String& url, const String& message) {
    return level + " " + clientIP + ": " + method + " " + url + " - " + message;
}

void LogHandler::logToSerial(const String &message) {
    dbSerial.println(message);
}

void LogHandler::logToFile(const String &message) {
    // Verifica se o arquivo de log existe e seu tamanho
    if (LittleFS.exists("/log.txt")) {
        File logFile = LittleFS.open("/log.txt", "r");
        if (logFile) {
            size_t fileSize = logFile.size();
            logFile.close();

            // Se o arquivo de log exceder o tamanho máximo, exclua-o
            if (fileSize > MAX_LOG_SIZE) {
                LittleFS.remove("/log.txt"); // Remove o arquivo de log antigo
            }
        }
    }

    // Abre o arquivo de log para adicionar a mensagem ou cria um novo se não existir
    File logFile = LittleFS.open("/log.txt", "a"); // Modo de append para adicionar ao final do arquivo
    if (!logFile) {
        dbSerial.println("Falha ao abrir arquivo de log para escrita no LogHandler!");
        return;
    }

    // Escreve a mensagem no arquivo de log e fecha o arquivo
    logFile.println(message);
    logFile.close();
}
