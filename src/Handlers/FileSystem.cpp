#include "FileSystem.h"
#include "SystemStatus.h"
#include <LittleFS.h>
#include "LogHandler.h"

extern LogHandler _logger;

bool FileSystem::loadConfig(SystemStatus& sysStat, const String& macAddress) {
    if (!LittleFS.begin(true)) {
        _logger.logError("Falha ao montar sistema de arquivos");
        return false;
    }
    
    // Tenta carregar configuração principal
    if (!loadConfigFile("/config.json", sysStat)) {
        _logger.logWarning("Falha ao carregar config principal, tentando backup");
        
        // Tenta carregar backup
        if (!loadConfigFile("/config.bak.json", sysStat)) {
            _logger.logError("Falha ao carregar backup, usando padrões");
            resetToDefaults(sysStat);
            return false;
        }
    }
    
    // Sempre mantém um backup da última configuração válida
    saveConfigFile("/config.bak.json", sysStat);
    return true;
} 