#include "OTAHandler.h"
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <MD5Builder.h>
#include "LogHandler.h"
#include <algorithm>

extern LogHandler logHandler;

// Configurações otimizadas
#define MIN_FREE_SPACE 65536  // 64KB mínimo livre
#define PROGRESS_INTERVAL 10  // Intervalo de 10% para logs
#define OTA_BUFFER_SIZE 4096
#define MAX_FIRMWARE_SIZE (4 * 1024 * 1024)  // 4MB máximo
#define UPDATE_TIMEOUT 300000  // 5 minutos timeout
#define MIN_HEAP_FOR_UPDATE 40000 // 40KB mínimo de heap livre para update
#define FIRMWARE_VERSION "1.0.0"  // Versão atual do firmware

OTAHandler::OTAHandler(LogHandler& logger) : _logger(logger) {
    _status.inProgress = false;
    _status.totalBytes = 0;
    _status.writtenBytes = 0;
    _status.startTime = 0;
    _status.currentVersion = String(ESP.getSdkVersion());
    _status.newVersion = "";
    _status.needsRollback = false;
    _status.progress = 0;
    _lastProgressUpdate = 0;
}

void OTAHandler::beginUpdate(size_t size, String version, String md5) {
    if (!hasEnoughSpace()) {
        logHandler.logError("Espaço insuficiente para atualização");
        return;
    }
    
    if (!isVersionNewer(version)) {
        logHandler.logError("Versão igual ou anterior à atual");
        return;
    }
    
    if (_status.inProgress) {
        logHandler.logError("Atualização já em andamento");
        return;
    }

    if (size > MAX_FIRMWARE_SIZE) {
        logHandler.logError("Firmware muito grande");
        return;
    }

    _status.inProgress = true;
    _status.totalBytes = size;
    _status.writtenBytes = 0;
    _status.startTime = millis();
    _status.newVersion = version;
    _status.progress = 0;
    _lastProgressUpdate = millis();

    // Backup do firmware atual
    _backupCurrentFirmware();

    // Inicia atualização
    if (!Update.begin(size)) {
        logHandler.logError("Não foi possível iniciar atualização");
        _status.inProgress = false;
        return;
    }

    // Se cliente enviou MD5 esperado, a lib Update verifica automaticamente no end()
    if (md5.length() == 32) {
        Update.setMD5(md5.c_str());
        logHandler.logMessage("MD5 esperado: " + md5);
    }

    logHandler.logMessage("Iniciando atualização OTA: " + version);
}

bool OTAHandler::writeUpdate(uint8_t* data, size_t len) {
    if (_checkTimeout()) {
        logHandler.logError("Timeout do OTA excedido");
        abortUpdate();
        return false;
    }

    const int MAX_RETRIES = 3;
    int retries = 0;

    while (retries < MAX_RETRIES) {
        if (Update.write(data, len) == len) {
            _status.writtenBytes += len;
            _updateProgress(_status.writtenBytes);
            return true;
        }
        
        logHandler.logError("Tentativa " + String(retries + 1) + " falhou");
        retries++;
        delay(100);  // Pequeno delay entre tentativas
    }
    
    logHandler.logError("Falha após " + String(MAX_RETRIES) + " tentativas");
    abortUpdate();
    return false;
}

bool OTAHandler::endUpdate() {
    if (!_status.inProgress) return false;

    if (!Update.end(false)) {
        logHandler.logError("Erro ao finalizar atualização: " + String(Update.errorString()));
        _status.needsRollback = true;
        return false;
    }

    if (!verifyFirmware()) {
        logHandler.logError("Verificação do firmware falhou");
        _status.needsRollback = true;
        return false;
    }

    logHandler.logMessage("Atualização concluída com sucesso");
    _status.inProgress = false;
    _status.needsRollback = false;
    return true;
}

void OTAHandler::abortUpdate() {
    Update.abort();
    _status.inProgress = false;
    _status.needsRollback = true;
    logHandler.logError("Atualização abortada");
}

bool OTAHandler::performRollback() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* previous = esp_ota_get_next_update_partition(NULL);

    if (!previous) {
        logHandler.logError("Partição anterior não encontrada");
        return false;
    }

    logHandler.logMessage("Iniciando rollback para partição anterior");
    if (esp_ota_set_boot_partition(previous) != ESP_OK) {
        logHandler.logError("Falha ao configurar partição de boot");
        return false;
    }

    logHandler.logMessage("Rollback concluído, reiniciando...");
    delay(1000);
    ESP.restart();
    return true;
}

bool OTAHandler::verifyFirmware() {
    const esp_partition_t* nextBoot = esp_ota_get_next_update_partition(NULL);
    if (!nextBoot) {
        logHandler.logError("Partição de atualização não encontrada");
        return false;
    }

    uint32_t magicNumber;
    if (esp_partition_read(nextBoot, 0, &magicNumber, sizeof(magicNumber)) != ESP_OK) {
        logHandler.logError("Erro ao ler assinatura do firmware");
        return false;
    }

    if (magicNumber != ESP_IMAGE_HEADER_MAGIC) {
        logHandler.logError("Assinatura do firmware inválida");
        return false;
    }

    return _verifyPartition(nextBoot);
}

void OTAHandler::checkRollbackNeeded() {
    if (_status.needsRollback) {
        logHandler.logMessage("Rollback necessário detectado");
        performRollback();
    }
}

OTAHandler::UpdateStatus OTAHandler::getStatus() {
    return _status;
}

bool OTAHandler::_verifyPartition(const esp_partition_t* partition) {
    if (!partition) return false;

    uint8_t buf[OTA_BUFFER_SIZE];
    MD5Builder md5;
    md5.begin();

    for (size_t i = 0; i < partition->size; i += OTA_BUFFER_SIZE) {
        size_t read_size = std::min(size_t(OTA_BUFFER_SIZE), size_t(partition->size - i));
        if (esp_partition_read(partition, i, buf, read_size) != ESP_OK) {
            return false;
        }
        md5.add(buf, read_size);
    }

    md5.calculate();
    logHandler.logMessage("MD5 do firmware: " + md5.toString());
    return true;
}

void OTAHandler::_backupCurrentFirmware() {
    const esp_partition_t* running = esp_ota_get_running_partition();
    const esp_partition_t* backup = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);

    if (!backup) {
        logHandler.logError("Partição de backup não encontrada");
        return;
    }

    logHandler.logMessage("Fazendo backup do firmware atual");
    if (esp_partition_erase_range(backup, 0, backup->size) != ESP_OK) {
        logHandler.logError("Erro ao apagar partição de backup");
        return;
    }

    uint8_t buffer[OTA_BUFFER_SIZE];
    for (size_t i = 0; i < running->size; i += OTA_BUFFER_SIZE) {
        size_t read_size = std::min(size_t(OTA_BUFFER_SIZE), size_t(running->size - i));
        if (esp_partition_read(running, i, buffer, read_size) != ESP_OK ||
            esp_partition_write(backup, i, buffer, read_size) != ESP_OK) {
            logHandler.logError("Erro ao copiar firmware para backup");
            return;
        }
    }

    logHandler.logMessage("Backup concluído com sucesso");
}

void OTAHandler::_updateProgress(size_t written) {
    unsigned long now = millis();
    if (now - _lastProgressUpdate > 1000) {  // Atualiza a cada segundo
        int newProgress = (written * 100) / _status.totalBytes;
        if (newProgress != _status.progress) {
            _status.progress = newProgress;
            logHandler.logMessage("Progresso: " + String(newProgress) + "%");
            _lastProgressUpdate = now;
        }
    }
}

bool OTAHandler::_checkTimeout() {
    return (millis() - _status.startTime) > UPDATE_TIMEOUT;
}

bool OTAHandler::hasEnoughSpace() {
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < MIN_HEAP_FOR_UPDATE) {
        logHandler.logError("Heap insuficiente para update: " + String(freeHeap) + " bytes");
        return false;
    }
    return true;
}

bool OTAHandler::isVersionNewer(const String& newVersion) {
    if (!_validateVersion(newVersion)) {
        logHandler.logError("Formato de versão inválido: " + newVersion);
        return false;
    }
    
    // Compara versões no formato x.y.z
    int current[3], newer[3];
    sscanf(FIRMWARE_VERSION, "%d.%d.%d", &current[0], &current[1], &current[2]);
    sscanf(newVersion.c_str(), "%d.%d.%d", &newer[0], &newer[1], &newer[2]);
    
    for (int i = 0; i < 3; i++) {
        if (newer[i] > current[i]) return true;
        if (newer[i] < current[i]) return false;
    }
    return false;
}

bool OTAHandler::_validateVersion(const String& version) {
    int major, minor, patch;
    return sscanf(version.c_str(), "%d.%d.%d", &major, &minor, &patch) == 3;
}
