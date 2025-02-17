#ifndef DIAGNOSTICS_HANDLER_H
#define DIAGNOSTICS_HANDLER_H

#include <Arduino.h>
#include "LogHandler.h"
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_task_wdt.h>

// Configurações do Watchdog
#define WDT_TIMEOUT_SECONDS 30
#define SOFT_WDT_INTERVAL 60000  // 60 segundos
#define MAX_TASK_BLOCKED_TIME 10000  // 10 segundos

class DiagnosticsHandler {
public:
    DiagnosticsHandler(LogHandler& logger);
    
    // Estrutura para armazenar métricas
    struct Metrics {
        size_t freeHeap;
        size_t minFreeHeap;
        size_t maxAllocHeap;
        float cpuFreqMHz;
        uint8_t freeStack;
        float heapFragmentation;
        uint32_t uptime;
        bool watchdogActive;
        unsigned long lastTaskCheck;
    };
    
    void logMetrics();
    Metrics getMetrics();
    void watchdogFeed();
    bool isHealthy();
    void enableWatchdog();
    void disableWatchdog();
    void checkTasks();
    void registerTaskCheck(TaskHandle_t task, const char* taskName);

private:
    LogHandler& _logger;
    unsigned long lastMetricsLog = 0;
    unsigned long lastWatchdogFeed = 0;
    const unsigned long METRICS_INTERVAL = 60000;
    bool watchdogEnabled = false;
    
    struct TaskInfo {
        TaskHandle_t handle;
        const char* name;
        unsigned long lastActiveTime;
    };
    std::vector<TaskInfo> monitoredTasks;
    
    void handleTaskTimeout(const TaskInfo& task);
    void initHardwareWatchdog();
    void initSoftwareWatchdog();
};

#endif 