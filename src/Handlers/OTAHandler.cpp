#include "OTAHandler.h"
#include <esp_ota_ops.h>

OTAHandler::OTAHandler() : totalReceived(0), lastSuccessfulIndex(0) {}

void OTAHandler::handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
        totalReceived = 0;  // Reinicia a contagem para novos uploads
        Serial.println("\nRecebendo novo arquivo de firmware: " + filename);
        if (filename.endsWith(".bin")) {
            Serial.println("Arquivo válido recebido. Iniciando o processo de upload.");
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { // Se o tamanho do update é desconhecido
                Update.printError(Serial);
                request->send(500, "text/plain", "Não foi possível iniciar o update");
                return;
            }
        } else {
            Serial.println("Arquivo inválido. O upload requer um arquivo .bin.");
            request->send(400, "text/plain", "400: Somente arquivos .bin são aceitos!");
            return;
        }
    }

    totalReceived += len;
    Serial.printf("Recebido %u de %u bytes\n", index + len, totalReceived); // Log de progresso

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
        request->send(500, "text/plain", "Erro durante o upload. Abortando...");
        Update.abort();
        return;
    }

    if (final) {
        if (Update.end(true)) { // True to set the size to the current progress
            Serial.printf("************Update Success: %uB *****************\n", totalReceived);
            request->send(200, "text/plain", "Update Success. Rebooting...");
            ESP.restart(); // Reinicia o dispositivo
        } else {
            Update.printError(Serial);
            request->send(500, "text/plain", "Falha na atualização. Tentando recuperar...");
            recoverToLastStableVersion();
        }
    }
}

void OTAHandler::recoverToLastStableVersion() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    if (esp_ota_set_boot_partition(next) == ESP_OK) {
        Serial.println("Recovery successful! Rebooting to the last known good firmware...");
        esp_restart();
    } else {
        Serial.println("Recovery failed! No valid firmware to revert to.");
    }
}
