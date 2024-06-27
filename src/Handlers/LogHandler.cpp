#include "LogHandler.h"
#include <FS.h>
#include <LittleFS.h>
#include <Nextion.h>


// Defina o tamanho máximo do arquivo de log em bytes (por exemplo, 1 MB)
const unsigned long MAX_LOG_SIZE = 1 * 1024 * 1024; // 1 MB

LogHandler::LogHandler() {}

void LogHandler::logMessage(const String& message) {
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