#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include "SystemStatus.h"

class FileSystem {
public:
    FileSystem();
    bool begin();
    static void initializeAndLoadConfig(SystemStatus &status, String mac);
    static void saveConfigToFile(const SystemStatus &status);
    static void verifyFileSystem();
    static void resetLogFile();

    String readFile(const char* path);
    bool writeFile(const char* path, const char* message);
    bool deleteFile(const char* path);
    void listDir(const char* dirname);
};

#endif /* FILESYSTEM_H */
