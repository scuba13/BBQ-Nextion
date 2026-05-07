#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Config.h"

class LogHandler {
private:
    static const size_t LOG_BUFFER_SIZE = LOG_BUFFER_BYTES;
    static const size_t MAX_LOG_SIZE    = LOG_MAX_FILE_SIZE;
    char logBuffer[LOG_BUFFER_SIZE];
    size_t bufferIndex = 0;
    unsigned long lastFlush = 0;
    const unsigned long FLUSH_INTERVAL  = LOG_FLUSH_INTERVAL;

    bool fileExists = false;
    size_t currentFileSize = 0;

    SemaphoreHandle_t _logMutex = nullptr;

    void flushBuffer();
    void checkFileSize();
    void rotateLogFile();

public:
    LogHandler();
    void begin();
    void logMessage(const String& message);
    void logWarning(const String &message);
    void logError(const String &message);
    void clearLogs();

    // Métodos adicionados de volta
    void logRequest(AsyncWebServerRequest *request, const String &message);

private:
    void writeLog(const String &level, const String &message);
};

#endif
