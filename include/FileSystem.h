#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include "SystemStatus.h"

class FileSystem {
public:
    FileSystem();
    bool begin();
    bool loadConfig(SystemStatus& sysStat, const String& macAddress);
    bool saveConfig(SystemStatus& sysStat);
    void resetToDefaults(SystemStatus& sysStat);
    static void initializeAndLoadConfig(SystemStatus &status, String mac);
    static void saveConfigToFile(const SystemStatus &status);
    static void verifyFileSystem();
    static void resetLogFile();  // Declaração do novo método

    // Novos métodos otimizados
    String readFile(const char* path);
    bool writeFile(const char* path, const char* message);
    bool deleteFile(const char* path);
    void listDir(const char* dirname);

private:
    bool loadConfigFile(const char* filename, SystemStatus& sysStat);
    bool saveConfigFile(const char* filename, SystemStatus& sysStat);

    // Cache de arquivos frequentes
    struct {
        String mqttConfig;
        String tempConfig;
        unsigned long lastRead = 0;
        const unsigned long CACHE_TIMEOUT = 60000; // 1 minuto
    } fsCache;
};

#endif /* FILESYSTEM_H */
