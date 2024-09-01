#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <Arduino.h>
#include "SystemStatus.h"
#include <sqlite3.h>

class FileSystem {
public:
    static void initializeAndLoadConfig(SystemStatus &status, String mac);
    static void saveConfigToDatabase(const char* key, const char* value, SystemStatus &systemStatus);
    static void verifyFileSystem();
    static void resetLogFile();

private:
    static void initializeDatabase(SystemStatus &status);
    static void insertDefaultConfigValues(sqlite3 *db, SystemStatus &status);
    static void loadConfigFromDatabase(SystemStatus &status, String mac);
    static void setStatusValue(SystemStatus &status, const String &key, const String &value);
};

#endif /* FILE_SYSTEM_H */
