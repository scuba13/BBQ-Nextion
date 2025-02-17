#include "OTAHandler.h"
#include <esp_ota_ops.h>
#include <Nextion.h>
#include "LogHandler.h"

extern LogHandler _logger;

// Configurações otimizadas
#define MIN_FREE_SPACE 65536  // 64KB mínimo livre
#define PROGRESS_INTERVAL 10  // Intervalo de 10% para logs

OTAHandler::OTAHandler() : totalReceived(0), lastSuccessfulIndex(0) {}

void OTAHandler::handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) { // Início do upload
        _logger.logMessage("Iniciando atualização OTA: " + filename);
        
        // Verifica espaço disponível
        if (ESP.getFreeSketchSpace() < MIN_FREE_SPACE) {
            request->send(500, "text/plain", "Espaço insuficiente");
            return;
        }

        // Configura Update com verificações de segurança
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH, LED_BUILTIN, true)) {
            Update.printError(Serial);
            request->send(500, "text/plain", "Falha ao iniciar OTA");
            return;
        }
    }

    // Processa dados recebidos com verificação
    if (!Update.write(data, len)) {
        Update.printError(Serial);
        request->send(500, "text/plain", "Falha ao escrever firmware");
        return;
    }
    
    totalReceived += len;
    lastSuccessfulIndex = index + len;

    // Log de progresso otimizado
    static int lastProgress = 0;
    int progress = (totalReceived * 100) / request->contentLength();
    if (progress - lastProgress >= PROGRESS_INTERVAL) {
        _logger.logMessage("Progresso OTA: " + String(progress) + "%");
        lastProgress = progress;
    }

    if (final) { // Upload finalizado
        if (Update.end(true)) {
            _logger.logMessage("OTA concluído com sucesso: " + String(totalReceived) + " bytes");
            request->send(200, "text/plain", "OK");
            delay(500);
            ESP.restart();
        } else {
            Update.printError(Serial);
            request->send(500, "text/plain", "Falha na finalização do OTA");
        }
    }
}

void OTAHandler::recoverToLastStableVersion() {
    // Implementação da recuperação usando ESP32 APIs nativas
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* next = esp_ota_get_next_update_partition(NULL);

    _logger.logMessage("Tentando recuperar última versão estável...");
    
    if (esp_ota_set_boot_partition(next) == ESP_OK) {
        _logger.logMessage("Recuperação bem sucedida");
        ESP.restart();
    } else {
        _logger.logMessage("Não foi possível recuperar versão anterior");
    }
}
