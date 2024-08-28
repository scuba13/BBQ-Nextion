#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <Nextion.h>

class LogHandler {
public:
    LogHandler();
    
    // Log a request with details about the client IP, HTTP method, and URL
    void logRequest(AsyncWebServerRequest *request, const String &message);
    
    // Log a generic message
    void logMessage(const String &message);
    
    // Log errors or warnings
    void logError(const String &message);

private:
    void logToSerial(const String &message);
    void logToFile(const String &message);
    String formatLogMessage(const String& level, const String& clientIP, const String& method, const String& url, const String& message);

    // Define o tamanho máximo do arquivo de log em bytes (por exemplo, 1 MB)
    const unsigned long MAX_LOG_SIZE = 1 * 1024 * 1024; // 1 MB
};

#endif // LOGGING_H
