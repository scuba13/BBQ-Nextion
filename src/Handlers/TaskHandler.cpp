#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>
#include <esp_task_wdt.h>
#include "LogHandler.h"
#include "NextionHandler.h"
#include "TemperatureControl.h"
#include "DiagnosticsHandler.h"

// Declarações externas
extern SystemStatus sysStat;
extern LogHandler logHandler;
extern DiagnosticsHandler diagnostics;  // Usar a instância global ao invés de criar uma nova

// Handles apenas para tasks necessárias
static TaskHandle_t tempTaskHandle = NULL;
static TaskHandle_t mqttTaskHandle = NULL;
static TaskHandle_t controlTaskHandle = NULL;
static TaskHandle_t diagnosticsTaskHandle = NULL;

// Referências globais
static SystemStatus* systemStatus;
static MQTTHandler* mqttHandler;

// Adicionar constantes para monitoramento
#define TASK_STACK_WATERMARK_THRESHOLD 512  // Alerta quando stack livre < 512 bytes
#define HEAP_WATERMARK_THRESHOLD 10000      // Alerta quando heap < 10KB

// Task para leitura de temperatura (500ms)
void temperatureTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(500);
    esp_task_wdt_add(NULL); // Registra esta task no hardware WDT

    UBaseType_t minStackLeft = UINT32_MAX;

    while (true) {
        esp_task_wdt_reset(); // Alimenta o WDT desta task

        UBaseType_t stackLeft = uxTaskGetStackHighWaterMark(NULL);
        if (stackLeft < minStackLeft) {
            minStackLeft = stackLeft;
            if (stackLeft < TASK_STACK_WATERMARK_THRESHOLD) {
                logHandler.logWarning("Stack baixa na TempTask: " + String(stackLeft) + " bytes");
            }
        }

        if (ESP.getFreeHeap() < HEAP_WATERMARK_THRESHOLD) {
            logHandler.logWarning("Heap baixa: " + String(ESP.getFreeHeap()) + " bytes");
        }

        getCalibratedTemp(thermocouple, *systemStatus);
        getCalibratedTempP(thermocoupleP, *systemStatus);
        getCalibratedInternalTemp(*systemStatus);
        vTaskDelay(xDelay);
    }
}

// Task para MQTT (3s)
void mqttTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(3000);
    while (true) {
        if (systemStatus->isHAAvailable) {
            mqttHandler->loop();
            mqttHandler->managePublishing(*systemStatus);
        }
        vTaskDelay(xDelay);
    }
}

// Task para controle de temperatura (1s)
void controlTask(void *parameter) {
    const TickType_t xDelay = pdMS_TO_TICKS(1000);
    esp_task_wdt_add(NULL); // Registra esta task no hardware WDT

    while (true) {
        esp_task_wdt_reset(); // Alimenta o WDT desta task
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
        logHandler.logMessage("MQTT task iniciada - HA disponível");
    } else {
        logHandler.logMessage("MQTT task não iniciada - HA não disponível");
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
        logHandler.logMessage("MQTT task iniciada posteriormente");
    }
}

// Função para parar MQTT
void stopMQTTTask() {
    if (mqttTaskHandle) {
        vTaskDelete(mqttTaskHandle);
        mqttTaskHandle = NULL;
        logHandler.logMessage("MQTT task parada");
    }
}

// Task de diagnóstico com intervalo de 60s
static DiagnosticsHandler* diagHandler = NULL;

void diagnosticsTaskFunc(void* parameter) {
    while (true) {
        diagHandler->checkTasks();
        diagHandler->logMetrics();
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void startDiagnosticsTask(DiagnosticsHandler& diag) {
    diagHandler = &diag;
    xTaskCreatePinnedToCore(
        diagnosticsTaskFunc,
        "DiagTask",
        2048,
        NULL,
        1,
        &diagnosticsTaskHandle,
        0
    );
    logHandler.logMessage("Diagnostics task iniciada");
}

