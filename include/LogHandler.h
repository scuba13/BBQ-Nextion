#ifndef LOG_HANDLER_H
#define LOG_HANDLER_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h> // Necessário para AsyncWebServerRequest

class LogHandler {
private:
    // Buffer circular para logs
    static const size_t LOG_BUFFER_SIZE = 1024;
    static const size_t MAX_LOG_SIZE = 50000; // 50KB máximo
    char logBuffer[LOG_BUFFER_SIZE];
    size_t bufferIndex = 0;
    unsigned long lastFlush = 0;
    const unsigned long FLUSH_INTERVAL = 5000; // 5 segundos

    // Cache de status do arquivo
    bool fileExists = false;
    size_t currentFileSize = 0;

    void flushBuffer();
    void checkFileSize();
    void rotateLogFile();
    String formatLogMessage(const String& level, const String& clientIP, const String& method, const String& url, const String& message);

public:
    LogHandler();
    void begin();
    void logMessage(const String& message);
    void clearLogs();

    // Métodos adicionados de volta
    void logRequest(AsyncWebServerRequest *request, const String &message);
    void logError(const String &message);
};

#endif
