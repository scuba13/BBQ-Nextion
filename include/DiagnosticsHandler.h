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

    // Contadores acumulados de eventos de falha
    struct HealthCounters {
        uint32_t wifiReconnections  = 0;
        uint32_t mqttReconnections  = 0;
        uint32_t sensorBBQErrors    = 0; // leituras MAX6675 BBQ rejeitadas
        uint32_t sensorPrtErrors    = 0; // leituras MAX6675 proteína rejeitadas
        uint32_t sensorIntErrors    = 0; // leituras DS18B20 rejeitadas
        uint32_t relayEmergencies   = 0; // acionamentos do failsafe de temperatura
    };

    void logMetrics();
    Metrics getMetrics();
    const HealthCounters& getCounters() const { return _counters; }
    void watchdogFeed();
    bool isHealthy();
    void enableWatchdog();
    void disableWatchdog();
    void checkTasks();
    void registerTaskCheck(TaskHandle_t task, const char* taskName);

    // Incrementadores chamados pelos subsistemas
    void countWifiReconnect()    { _counters.wifiReconnections++; }
    void countMqttReconnect()    { _counters.mqttReconnections++; }
    void countSensorBBQError()   { _counters.sensorBBQErrors++; }
    void countSensorPrtError()   { _counters.sensorPrtErrors++; }
    void countSensorIntError()   { _counters.sensorIntErrors++; }
    void countRelayEmergency()   { _counters.relayEmergencies++; }

private:
    LogHandler& _logger;
    unsigned long lastMetricsLog = 0;
    unsigned long lastWatchdogFeed = 0;
    const unsigned long METRICS_INTERVAL = 60000;
    bool watchdogEnabled = false;
    HealthCounters _counters;
    
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