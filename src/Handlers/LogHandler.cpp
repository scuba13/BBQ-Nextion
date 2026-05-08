#include "Handlers/LogHandler.h"
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

LogHandler::LogHandler() {
    memset(logBuffer, 0, LOG_BUFFER_SIZE);
    _logMutex = xSemaphoreCreateMutex();
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
    if (_logMutex != nullptr) xSemaphoreTake(_logMutex, pdMS_TO_TICKS(200));

    if (LittleFS.exists("/log.txt")) LittleFS.remove("/log.txt");
    if (LittleFS.exists("/log.old")) LittleFS.remove("/log.old");

    File file = LittleFS.open("/log.txt", "w");
    if (file) {
        file.close();
        currentFileSize = 0;
    }

    memset(logBuffer, 0, LOG_BUFFER_SIZE);
    bufferIndex = 0;
    lastFlush = millis();

    if (_logMutex != nullptr) xSemaphoreGive(_logMutex);
}

void LogHandler::writeLog(const String &level, const String &message) {
    if (_logMutex == nullptr || xSemaphoreTake(_logMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.printf("[NO-MUTEX][%s] %s\n", level.c_str(), message.c_str());
        return;
    }

    unsigned long now = millis();
    char msgBuf[256];
    int len;

    // Usa timestamp NTP se sincronizado, senão usa millis()
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0)) {
        len = snprintf(msgBuf, sizeof(msgBuf), "%02d/%02d %02d:%02d:%02d [%s] %s\n",
                       timeinfo.tm_mday, timeinfo.tm_mon + 1,
                       timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                       level.c_str(), message.c_str());
    } else {
        len = snprintf(msgBuf, sizeof(msgBuf), "%lus: [%s] %s\n",
                       now / 1000UL, level.c_str(), message.c_str());
    }
    if (len < 0) len = 0;
    if (len >= (int)sizeof(msgBuf)) len = (int)sizeof(msgBuf) - 1;

    Serial.print(msgBuf);

    if (bufferIndex + (size_t)len >= LOG_BUFFER_SIZE) {
        flushBuffer();
    }

    memcpy(logBuffer + bufferIndex, msgBuf, len);
    bufferIndex += len;

    if (now - lastFlush >= FLUSH_INTERVAL) {
        flushBuffer();
    }

    xSemaphoreGive(_logMutex);
}
