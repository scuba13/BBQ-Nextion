#ifndef WebServerControl_h
#define WebServerControl_h


#include "SystemStatus.h"
#include "FileSystem.h"
#include "LogHandler.h"
#include "OTAHandler.h"
#include "DiagnosticsHandler.h"

class WebServerControl {
public:
    WebServerControl(SystemStatus& systemStatus, FileSystem& fileSystem, LogHandler& logger, AsyncWebServer& server, DiagnosticsHandler& diagnosticsHandler);
    void begin();

private:
    SystemStatus& _systemStatus;
    FileSystem& _fileSystem;
    LogHandler& _logger;
    AsyncWebServer& _server; // Usando referência para o servidor
    OTAHandler _otaHandler;  // Instância com inicialização no construtor
    DiagnosticsHandler& _diagnostics;  // Renomeado para seguir padrão
};

#endif
