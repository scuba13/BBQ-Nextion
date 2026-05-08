#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <Update.h>
#include <ESPAsyncWebServer.h>
#include "Handlers/LogHandler.h"
#include "Config.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>

#define OTA_BUFFER_SIZE 4096

class OTAHandler {
public:
    OTAHandler(LogHandler& logger);

    // Estrutura para controle de atualização
    struct UpdateStatus {
        bool inProgress;
        size_t totalBytes;
        size_t writtenBytes;
        uint32_t startTime;
        String currentVersion;
        String newVersion;
        bool needsRollback;
        int progress;
    };

    void beginUpdate(size_t size, String version, String md5 = "");
    bool writeUpdate(uint8_t* data, size_t len);
    bool endUpdate();
    void abortUpdate();
    bool performRollback();
    bool verifyFirmware();
    UpdateStatus getStatus();
    void checkRollbackNeeded();
    bool hasEnoughSpace();
    String getFirmwareVersion() { return FIRMWARE_VERSION; }
    bool isVersionNewer(const String& newVersion);
    
private:
    LogHandler& _logger;
    UpdateStatus _status;
    uint32_t _lastProgressUpdate;
    bool _verifyPartition(const esp_partition_t* partition);
    void _backupCurrentFirmware();
    void _updateProgress(size_t written);
    bool _checkTimeout();
    bool _validateVersion(const String& version);
};

#endif // OTA_HANDLER_H
