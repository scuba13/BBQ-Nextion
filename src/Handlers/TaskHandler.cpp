#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>
#include "AppContext.h"

// Constantes para configuração das tasks
constexpr uint32_t STACK_SIZE_TEMP = 4000;
constexpr uint32_t STACK_SIZE_CONTROL = 4000;
constexpr uint32_t TASK_PRIORITY = 1;
constexpr TickType_t TEMP_READ_DELAY = pdMS_TO_TICKS(500);
constexpr TickType_t CONTROL_DELAY = pdMS_TO_TICKS(1000);

// Estrutura auxiliar para configuração de tasks
struct TaskConfig_t {
    const char* name;
    TaskFunction_t function;
    const uint32_t stackSize;
};

// Cache de temperatura para reduzir leituras
static struct {
    unsigned long lastRead = 0;
    float bbqTemp = 0;
    float proteinTemp = 0;
} tempCache;

// Tarefa para obter a temperatura calibrada do termopar
void getCalibratedTempTask(void *parameter)
{
    app.logHandler.logMessage("Task getCalibratedTempTask started.");
    while (true)
    {
        unsigned long now = millis();
        // Atualizar cache apenas a cada 250ms
        if (now - tempCache.lastRead >= 250) {
            tempCache.bbqTemp = getCalibratedTemp(thermocouple, app.sysStat);
            tempCache.lastRead = now;
        }
        
        vTaskDelay(TEMP_READ_DELAY);
    }
}

// Tarefa para obter a temperatura calibrada do termopar de proteína
void getCalibratedTempPTask(void *parameter)
{
    app.logHandler.logMessage("Task getCalibratedTempPTask started.");
    while (true)
    {
        getCalibratedTempP(thermocoupleP, app.sysStat);
        vTaskDelay(TEMP_READ_DELAY);
    }
}

// Tarefa para controlar a temperatura
void controlTemperatureTask(void *parameter)
{
    app.logHandler.logMessage("Task controlTemperatureTask started.");
    static unsigned long lastControl = 0;
    
    while (true)
    {
        if (app.sysStat.bbqTemperature > 0)
        {
            unsigned long now = millis();
            // Controlar temperatura apenas se necessário
            if (now - lastControl >= 100) { // 100ms intervalo mínimo
                controlTemperature(app.sysStat);
                lastControl = now;
            }
        }
        vTaskDelay(CONTROL_DELAY);
    }
}

// Tarefa para obter a temperatura calibrada do ds18b20
void getCalibratedInternalTempTask(void *parameter)
{
    app.logHandler.logMessage("Task getCalibratedInternalTempTask started.");
    while (true)
    {
        //getCalibratedInternalTemp(sysStat);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Função para criar as tarefas
void createTasks()
{
    // Array de configurações das tasks
    static const TaskConfig_t taskConfigs[] = {
        {"TempTask", getCalibratedTempTask, STACK_SIZE_TEMP},
        {"TempPTask", getCalibratedTempPTask, STACK_SIZE_TEMP},
        {"ControlTask", controlTemperatureTask, STACK_SIZE_CONTROL}
    };
    
    app.logHandler.logMessage("Creating tasks...");
    
    // Criar cada task baseado na configuração
    for (const auto& config : taskConfigs) {
        BaseType_t result = xTaskCreate(
            config.function,
            config.name,
            config.stackSize,
            nullptr,
            TASK_PRIORITY,
            nullptr
        );
        
        if (result == pdPASS) {
            app.logHandler.logMessage(String(config.name) + " created");
        } else {
            app.logHandler.logMessage(String(config.name) + " creation failed");
        }
    }
    
    app.logHandler.logMessage("All tasks created");
}
