#ifndef SYSTAT_MUTEX_H
#define SYSTAT_MUTEX_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Mutex recursivo global para sysStat.
// Recursivo porque resetSystem() pode ser chamado de dentro de callbacks
// que já seguram o mutex (ex: mqttTask → messageHandler → resetSystem).
extern SemaphoreHandle_t sysStatMutex;

inline void sysStatLock() {
    if (sysStatMutex != nullptr) xSemaphoreTakeRecursive(sysStatMutex, portMAX_DELAY);
}

inline void sysStatUnlock() {
    if (sysStatMutex != nullptr) xSemaphoreGiveRecursive(sysStatMutex);
}

#endif // SYSTAT_MUTEX_H
