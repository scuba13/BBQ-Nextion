#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>
#include "LogHandler.h"
#include "NextionHandler.h"
#include "TemperatureControl.h"
#include "DiagnosticsHandler.h"

// Declarações externas
extern SystemStatus sysStat;
extern LogHandler _logger;
extern DiagnosticsHandler diagnostics;  // Usar a instância global ao invés de criar uma nova

// Configurações otimizadas para as tasks
#define TEMP_TASK_STACK    3072  // Reduzido de 4000
#define TEMP_TASK_PRIORITY 2     // Aumentado para prioridade maior
#define TEMP_TASK_CORE     1     // Core dedicado para temperatura

#define CONTROL_TASK_STACK    2048  // Reduzido de 4000
#define CONTROL_TASK_PRIORITY 1     // Prioridade menor
#define CONTROL_TASK_CORE     0     // Core separado para controle

// Handles apenas para tasks necessárias
static TaskHandle_t tempTaskHandle = NULL;
static TaskHandle_t mqttTaskHandle = NULL;
static TaskHandle_t controlTaskHandle = NULL;

// Referências globais
static SystemStatus* systemStatus;
static MQTTHandler* mqttHandler;

// Adicionar constantes para monitoramento
#define TASK_STACK_WATERMARK_THRESHOLD 512  // Alerta quando stack livre < 512 bytes
#define HEAP_WATERMARK_THRESHOLD 10000      // Alerta quando heap < 10KB

// Task para leitura de temperatura (500ms)
void temperatureTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(500);
    
    // Monitor de stack
    UBaseType_t minStackLeft = UINT32_MAX;
    
    while (true) {
        // Verifica stack disponível
        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        if (stackLeft < minStackLeft) {
            minStackLeft = stackLeft;
            if (stackLeft < TASK_STACK_WATERMARK_THRESHOLD) {
                _logger.logWarning("Stack baixa na TempTask: " + String(stackLeft) + " bytes");
            }
        }
        
        // Verifica heap
        if (ESP.getFreeHeap() < HEAP_WATERMARK_THRESHOLD) {
            _logger.logWarning("Heap baixa: " + String(ESP.getFreeHeap()) + " bytes");
        }
        
        getCalibratedTemp(thermocouple, *systemStatus);
        getCalibratedTempP(thermocoupleP, *systemStatus);
        getCalibratedInternalTemp(*systemStatus);
        vTaskDelay(xDelay);
    }
}

// Task para MQTT (3s) - Agora com verificação de disponibilidade
void mqttTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(3000);
    while (true) {
        if (systemStatus->isHAAvailable) {  // Só executa se MQTT estiver disponível
            mqttHandler->managePublishing(*systemStatus);
        }
        vTaskDelay(xDelay);
    }
}

// Task para controle de temperatura (1s)
void controlTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    while (true) {
        controlTemperature(*systemStatus);
        vTaskDelay(xDelay);
    }
}

void initializeTasks(SystemStatus& sysStat, MQTTHandler& mqtt) {
    systemStatus = &sysStat;
    mqttHandler = &mqtt;

    // Task de temperatura no core 0
    xTaskCreatePinnedToCore(
        temperatureTask,
        "TempTask",
        4096,
        NULL,
        2, // Prioridade alta
        &tempTaskHandle,
        0
    );

    // Task de controle no core 0
    xTaskCreatePinnedToCore(
        controlTask,
        "ControlTask",
        2048,
        NULL,
        1, // Prioridade média
        &controlTaskHandle,
        0
    );

    // Cria task MQTT apenas se estiver disponível
    if (sysStat.isHAAvailable) {
        xTaskCreatePinnedToCore(
            mqttTask,
            "MQTTTask",
            4096,
            NULL,
            1, // Prioridade baixa
            &mqttTaskHandle,
            1
        );
        _logger.logMessage("MQTT task iniciada - HA disponível");
    } else {
        _logger.logMessage("MQTT task não iniciada - HA não disponível");
    }

    // Registra tasks para monitoramento
    diagnostics.registerTaskCheck(tempTaskHandle, "TempTask");
    diagnostics.registerTaskCheck(controlTaskHandle, "ControlTask");
    if (mqttTaskHandle) {
        diagnostics.registerTaskCheck(mqttTaskHandle, "MQTTTask");
    }
}

void stopTasks() {
    if (tempTaskHandle) vTaskDelete(tempTaskHandle);
    if (controlTaskHandle) vTaskDelete(controlTaskHandle);
    if (mqttTaskHandle) {  // Só tenta parar se existir
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = NULL;
    }
}

// Função para iniciar MQTT posteriormente
void startMQTTTask() {
    if (!mqttTaskHandle && systemStatus->isHAAvailable) {
        xTaskCreatePinnedToCore(
            mqttTask,
            "MQTTTask",
            4096,
            NULL,
            1,
            &mqttTaskHandle,
            1
        );
        _logger.logMessage("MQTT task iniciada posteriormente");
    }
}

// Função para parar MQTT
void stopMQTTTask() {
    if (mqttTaskHandle) {
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = NULL;
        _logger.logMessage("MQTT task parada");
    }
}

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
