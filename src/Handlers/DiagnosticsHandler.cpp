#include "DiagnosticsHandler.h"
#include <esp_int_wdt.h>
#include <esp_system.h>

DiagnosticsHandler::DiagnosticsHandler(LogHandler& logger) : _logger(logger) {
    initHardwareWatchdog();
    initSoftwareWatchdog();
}

void DiagnosticsHandler::initHardwareWatchdog() {
    esp_task_wdt_init(WDT_TIMEOUT_SECONDS, true); // true para panic mode
    esp_task_wdt_add(NULL); // Adiciona task atual
    _logger.logMessage("Hardware Watchdog iniciado");
}

void DiagnosticsHandler::initSoftwareWatchdog() {
    watchdogEnabled = true;
    lastWatchdogFeed = millis();
    _logger.logMessage("Software Watchdog iniciado");
}

void DiagnosticsHandler::watchdogFeed() {
    if (!watchdogEnabled) return;
    
    unsigned long now = millis();
    
    // Alimenta hardware watchdog
    esp_task_wdt_reset();
    
    // Verifica software watchdog
    if (now - lastWatchdogFeed > SOFT_WDT_INTERVAL) {
        _logger.logError("Software Watchdog timeout detectado!");
        ESP.restart();
    }
    
    lastWatchdogFeed = now;
}

void DiagnosticsHandler::registerTaskCheck(TaskHandle_t task, const char* taskName) {
    TaskInfo info = {
        .handle = task,
        .name = taskName,
        .lastActiveTime = millis()
    };
    monitoredTasks.push_back(info);
    _logger.logMessage("Task registrada para monitoramento: " + String(taskName));
}

void DiagnosticsHandler::checkTasks() {
    unsigned long now = millis();
    
    for (auto& task : monitoredTasks) {
        if (eTaskGetState(task.handle) == eBlocked) {
            if (now - task.lastActiveTime > MAX_TASK_BLOCKED_TIME) {
                handleTaskTimeout(task);
            }
        } else {
            task.lastActiveTime = now;
        }
    }
}

void DiagnosticsHandler::handleTaskTimeout(const TaskInfo& task) {
    String error = "Task bloqueada detectada: " + String(task.name);
    error += " - Tempo: " + String((millis() - task.lastActiveTime) / 1000) + "s";
    _logger.logError(error);
    
    // Tenta recuperar a task
    vTaskDelete(task.handle);
    
    // Se for uma task crítica, reinicia o sistema
    if (String(task.name) == "TempTask" || String(task.name) == "ControlTask") {
        _logger.logError("Task crítica falhou - Reiniciando sistema");
        ESP.restart();
    }
}

void DiagnosticsHandler::enableWatchdog() {
    watchdogEnabled = true;
    _logger.logMessage("Watchdog habilitado");
}

void DiagnosticsHandler::disableWatchdog() {
    watchdogEnabled = false;
    _logger.logMessage("Watchdog desabilitado");
}

void DiagnosticsHandler::logMetrics() {
    unsigned long now = millis();
    if (now - lastMetricsLog >= METRICS_INTERVAL) {
        Metrics metrics = getMetrics();
        
        String report = "=== Diagnóstico do Sistema ===\n";
        report += "Heap Livre: " + String(metrics.freeHeap) + " bytes\n";
        report += "Heap Mínimo: " + String(metrics.minFreeHeap) + " bytes\n";
        report += "Maior Alocação: " + String(metrics.maxAllocHeap) + " bytes\n";
        report += "CPU Freq: " + String(metrics.cpuFreqMHz) + " MHz\n";
        report += "Stack Livre: " + String(metrics.freeStack) + "%\n";
        report += "Fragmentação: " + String(metrics.heapFragmentation) + "%\n";
        report += "Uptime: " + String(metrics.uptime) + " segundos\n";
        
        _logger.logMessage(report);
        lastMetricsLog = now;
    }
}

DiagnosticsHandler::Metrics DiagnosticsHandler::getMetrics() {
    Metrics metrics;
    
    // Heap
    metrics.freeHeap = esp_get_free_heap_size();
    metrics.minFreeHeap = esp_get_minimum_free_heap_size();
    metrics.maxAllocHeap = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    
    // CPU
    metrics.cpuFreqMHz = ESP.getCpuFreqMHz();
    
    // Stack
    metrics.freeStack = uxTaskGetStackHighWaterMark(NULL);
    
    // Fragmentação
    metrics.heapFragmentation = 100 - ((float)metrics.maxAllocHeap * 100) / metrics.freeHeap;
    
    // Uptime
    metrics.uptime = millis() / 1000;
    
    return metrics;
}

bool DiagnosticsHandler::isHealthy() {
    Metrics metrics = getMetrics();
    
    // Critérios de saúde do sistema
    bool heapOk = metrics.freeHeap > 10000; // Mínimo 10KB livre
    bool fragOk = metrics.heapFragmentation < 70; // Máximo 70% fragmentação
    bool stackOk = metrics.freeStack > 20; // Mínimo 20% stack livre
    
    return heapOk && fragOk && stackOk;
} 