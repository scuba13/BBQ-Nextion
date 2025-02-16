#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <Update.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LogHandler.h"

class OTAHandler {
public:
    OTAHandler(LogHandler& logger);
    
    // Manipula o upload do firmware
    void handleFirmwareUpload(AsyncWebServerRequest *request, 
                             String filename, 
                             size_t index, 
                             uint8_t *data, 
                             size_t len, 
                             bool final);

    // Verifica a integridade do firmware
    bool verifyFirmware();
    
    // Recupera para a última versão estável
    bool recoverToLastStableVersion();

private:
    LogHandler& logger;
    size_t totalSize;
    size_t writtenSize;
    bool updateStarted;
    uint32_t lastProgressUpdate;
    static constexpr size_t BUFFER_SIZE = 4096;
    static constexpr uint32_t PROGRESS_INTERVAL = 1000; // 1 segundo

    // Métodos auxiliares
    bool beginUpdate(const String& filename, AsyncWebServerRequest *request);
    bool writeUpdate(uint8_t *data, size_t len);
    bool finalizeUpdate(AsyncWebServerRequest *request);
    void sendResponse(AsyncWebServerRequest *request, int code, const char* message);
    void updateProgress();
};

#endif // OTA_HANDLER_H
