#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <Arduino.h>
#include "SystemStatus.h"

class FileSystem {
public:
    static void initializeAndLoadConfig(SystemStatus &status, String mac);
    static void saveConfigToFile(const SystemStatus &status);
    static void verifyFileSystem();
    static void resetLogFile();  // Declaração do novo método
};

#endif /* FILE_SYSTEM_H */
