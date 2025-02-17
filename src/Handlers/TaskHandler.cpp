#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>
#include "LogHandler.h"

// Declarações externas
extern SystemStatus sysStat;
extern LogHandler _logger;

// Configurações otimizadas para as tasks
#define TEMP_TASK_STACK    3072  // Reduzido de 4000
#define TEMP_TASK_PRIORITY 2     // Aumentado para prioridade maior
#define TEMP_TASK_CORE     1     // Core dedicado para temperatura

#define CONTROL_TASK_STACK    2048  // Reduzido de 4000
#define CONTROL_TASK_PRIORITY 1     // Prioridade menor
#define CONTROL_TASK_CORE     0     // Core separado para controle

// Task de temperatura do BBQ - otimizada
void getCalibratedTempTask(void *parameter) {
    const TickType_t xFrequency = pdMS_TO_TICKS(500); // 2Hz é suficiente
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        getCalibratedTemp(thermocouple, sysStat);
        vTaskDelayUntil(&xLastWakeTime, xFrequency); // Timing mais preciso
    }
}

// Task de temperatura da sonda - otimizada
void getCalibratedTempPTask(void *parameter) {
    const TickType_t xFrequency = pdMS_TO_TICKS(500);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        getCalibratedTempP(thermocoupleP, sysStat);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Task de controle - otimizada
void controlTemperatureTask(void *parameter) {
    const TickType_t xFrequency = pdMS_TO_TICKS(1000); // 1Hz para controle
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (true) {
        if (sysStat.bbqTemperature > 0) {
            controlTemperature(sysStat);
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Tarefa para obter a temperatura calibrada do ds18b20
void getCalibratedInternalTempTask(void *parameter)
{
    _logger.logMessage("Task getCalibratedInternalTempTask started.");
    while (true)
    {
        //getCalibratedInternalTemp(sysStat);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Função de criação das tasks otimizada
void createTasks() {
    _logger.logMessage("Iniciando criação das tasks...");

    // Task de temperatura BBQ
    xTaskCreatePinnedToCore(
        getCalibratedTempTask,
        "TempTask",
        TEMP_TASK_STACK,
        NULL,
        TEMP_TASK_PRIORITY,
        NULL,
        TEMP_TASK_CORE
    );

    // Task de temperatura Sonda
    xTaskCreatePinnedToCore(
        getCalibratedTempPTask,
        "TempPTask",
        TEMP_TASK_STACK,
        NULL,
        TEMP_TASK_PRIORITY,
        NULL,
        TEMP_TASK_CORE
    );

    // Task de controle
    xTaskCreatePinnedToCore(
        controlTemperatureTask,
        "ControlTask",
        CONTROL_TASK_STACK,
        NULL,
        CONTROL_TASK_PRIORITY,
        NULL,
        CONTROL_TASK_CORE
    );

    _logger.logMessage("Tasks criadas com sucesso");
}
