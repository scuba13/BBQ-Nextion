#include "OTAHandler.h"
#include <esp_ota_ops.h>
#include <esp_partition.h>

OTAHandler::OTAHandler(LogHandler& l) 
    : logger(l), totalSize(0), writtenSize(0), updateStarted(false), lastProgressUpdate(0) {}

void OTAHandler::handleFirmwareUpload(AsyncWebServerRequest *request, 
                                    String filename, 
                                    size_t index, 
                                    uint8_t *data, 
                                    size_t len, 
                                    bool final) {
    if (!index) {
        if (!beginUpdate(filename, request)) {
            return;
        }
    }

    if (!writeUpdate(data, len)) {
        sendResponse(request, 500, "Failed to write firmware chunk");
        Update.abort();
        return;
    }

    if (final) {
        if (!finalizeUpdate(request)) {
            recoverToLastStableVersion();
        }
    }
}

bool OTAHandler::beginUpdate(const String& filename, AsyncWebServerRequest *request) {
    if (!filename.endsWith(".bin")) {
        sendResponse(request, 400, "Invalid file type. Only .bin files are accepted");
        return false;
    }

    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        logger.logError("OTA: Failed to begin update");
        sendResponse(request, 500, "Failed to begin update");
        return false;
    }

    updateStarted = true;
    totalSize = 0;
    writtenSize = 0;
    lastProgressUpdate = 0;
    logger.logMessage("OTA: Update started");
    return true;
}

bool OTAHandler::writeUpdate(uint8_t *data, size_t len) {
    if (!updateStarted) {
        return false;
    }

    if (Update.write(data, len) != len) {
        logger.logError("OTA: Write failed");
        return false;
    }

    writtenSize += len;
    updateProgress();
    return true;
}

bool OTAHandler::finalizeUpdate(AsyncWebServerRequest *request) {
    if (!Update.end(true)) {
        logger.logError("OTA: Update end failed");
        sendResponse(request, 500, "Update finalization failed");
        return false;
    }

    if (!verifyFirmware()) {
        logger.logError("OTA: Firmware verification failed");
        sendResponse(request, 500, "Firmware verification failed");
        return false;
    }

    logger.logMessage("OTA: Update successful");
    sendResponse(request, 200, "Update successful. Rebooting...");
    delay(1000);
    ESP.restart();
    return true;
}

bool OTAHandler::verifyFirmware() {
    if (!Update.isFinished()) {
        return false;
    }

    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* update = esp_ota_get_next_update_partition(NULL);
    
    if (!running || !update) {
        return false;
    }

    return true;
}

bool OTAHandler::recoverToLastStableVersion() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* last_valid = esp_ota_get_last_invalid_partition();
    
    if (!running || !last_valid) {
        logger.logError("OTA: No valid partition found for recovery");
        return false;
    }

    if (esp_ota_set_boot_partition(last_valid) != ESP_OK) {
        logger.logError("OTA: Failed to set recovery partition");
        return false;
    }

    logger.logMessage("OTA: Recovery successful, rebooting...");
    delay(1000);
    ESP.restart();
    return true;
}

void OTAHandler::sendResponse(AsyncWebServerRequest *request, int code, const char* message) {
    AsyncWebServerResponse *response = request->beginResponse(code, "text/plain", message);
    response->addHeader("Connection", "close");
    request->send(response);
}

void OTAHandler::updateProgress() {
    uint32_t now = millis();
    if (now - lastProgressUpdate >= PROGRESS_INTERVAL) {
        int progress = (writtenSize * 100) / totalSize;
        logger.logMessage("OTA Progress: " + String(progress) + "%");
        lastProgressUpdate = now;
    }
}
