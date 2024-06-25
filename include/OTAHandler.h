#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <Arduino.h>
#include <Update.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

class OTAHandler {
public:
    OTAHandler();  // Construtor
    void handleFirmwareUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void recoverToLastStableVersion();  // Método para recuperar para a última versão estável

private:
    size_t totalReceived;  // Total de bytes recebidos durante a sessão de upload atual
    size_t lastSuccessfulIndex;  // Índice do último byte bem-sucedido recebido (pode ser usado para recuperação se necessário)
};

#endif // OTA_HANDLER_H
