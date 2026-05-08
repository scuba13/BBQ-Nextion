#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "SystemStatus.h"
#include "Handlers/MQTTHandler.h"
#include "Handlers/DiagnosticsHandler.h"

struct TaskStackInfo {
    UBaseType_t tempTask;    // bytes livres — TempTask
    UBaseType_t controlTask; // bytes livres — ControlTask
    UBaseType_t mqttTask;    // 0 se a task não está rodando
};

// Retorna o high-watermark de stack das três tasks principais
TaskStackInfo getTaskStackWatermarks();

// Inicializa as tasks principais
void initializeTasks(SystemStatus& sysStat, MQTTHandler& mqtt);

// Para as tasks em execução
void stopTasks();

// Controle específico do MQTT
void startMQTTTask();
void stopMQTTTask();

// Task de diagnóstico (60s)
void startDiagnosticsTask(DiagnosticsHandler& diag);

#endif
