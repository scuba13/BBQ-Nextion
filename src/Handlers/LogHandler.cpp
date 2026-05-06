#include "LogHandler.h"
#include <FS.h>
#include <LittleFS.h>

LogHandler::LogHandler() {
    memset(logBuffer, 0, LOG_BUFFER_SIZE);
}

void LogHandler::begin() {
    if (!LittleFS.exists("/log.txt")) {
        File file = LittleFS.open("/log.txt", "w");
        if (file) {
            file.close();
            fileExists = true;
            currentFileSize = 0;
        }
    } else {
        File file = LittleFS.open("/log.txt", "r");
        if (file) {
            currentFileSize = file.size();
            file.close();
            fileExists = true;
        }
    }
}

void LogHandler::logRequest(AsyncWebServerRequest *request, const String &message) {
    String info = request->client()->remoteIP().toString() + " " +
                  request->methodToString() + " " +
                  request->url() + " - " + message;
    writeLog("REQUEST", info);
}

void LogHandler::logMessage(const String& message) {
    writeLog("INFO", message);
}

void LogHandler::logError(const String &message) {
    writeLog("ERROR", message);
}

void LogHandler::logWarning(const String &message) {
    writeLog("WARN", message);
}

void LogHandler::flushBuffer() {
    if (bufferIndex == 0) return;
    
    checkFileSize();
    
    File file = LittleFS.open("/log.txt", "a");
    if (!file) return;
    
    file.write((uint8_t*)logBuffer, bufferIndex);
    currentFileSize += bufferIndex;
    
    file.close();
    memset(logBuffer, 0, LOG_BUFFER_SIZE);
    bufferIndex = 0;
    lastFlush = millis();
}

void LogHandler::checkFileSize() {
    if (currentFileSize + bufferIndex > MAX_LOG_SIZE) {
        rotateLogFile();
    }
}

void LogHandler::rotateLogFile() {
    // Remove arquivo antigo de backup se existir
    if (LittleFS.exists("/log.old")) {
        LittleFS.remove("/log.old");
    }
    
    // Renomeia arquivo atual para backup
    if (LittleFS.exists("/log.txt")) {
        LittleFS.rename("/log.txt", "/log.old");
    }
    
    // Cria novo arquivo
    File file = LittleFS.open("/log.txt", "w");
    if (file) {
        file.close();
        currentFileSize = 0;
    }
}

void LogHandler::clearLogs() {
    if (LittleFS.exists("/log.txt")) {
        LittleFS.remove("/log.txt");
    }
    if (LittleFS.exists("/log.old")) {
        LittleFS.remove("/log.old");
    }
    
    File file = LittleFS.open("/log.txt", "w");
    if (file) {
        file.close();
        currentFileSize = 0;
    }
    
    memset(logBuffer, 0, LOG_BUFFER_SIZE);
    bufferIndex = 0;
    lastFlush = millis();
}

void LogHandler::writeLog(const String &level, const String &message) {
    // Formata a mensagem com timestamp e nível
    unsigned long now = millis();
    String timeStamp = String(now/1000) + "s: ";
    String fullMessage = timeStamp + "[" + level + "] " + message + "\n";
    
    // Imprime no Serial
    Serial.print(fullMessage);
    
    // Verifica se há espaço no buffer
    if (bufferIndex + fullMessage.length() >= LOG_BUFFER_SIZE) {
        flushBuffer();
    }
    
    // Adiciona ao buffer
    memcpy(logBuffer + bufferIndex, fullMessage.c_str(), fullMessage.length());
    bufferIndex += fullMessage.length();
    
    // Verifica se é hora de fazer flush
    if (now - lastFlush >= FLUSH_INTERVAL) {
        flushBuffer();
    }
}
