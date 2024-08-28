#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>
#include "LogHandler.h"

// Declaração externa de sysStat para que as tarefas possam acessar
extern SystemStatus sysStat;
extern LogHandler _logger; // Certifique-se de que o logHandler esteja declarado externamente ou passado como argumento

// Tarefa para obter a temperatura calibrada do termopar
void getCalibratedTempTask(void *parameter)
{
    _logger.logMessage("Task getCalibratedTempTask started.");
    while (true)
    {
        getCalibratedTemp(thermocouple, sysStat);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

// Tarefa para obter a temperatura calibrada do termopar de proteína
void getCalibratedTempPTask(void *parameter)
{
    _logger.logMessage("Task getCalibratedTempPTask started.");
    while (true)
    {
        getCalibratedTempP(thermocoupleP, sysStat);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Tarefa para controlar a temperatura
void controlTemperatureTask(void *parameter)
{
    _logger.logMessage("Task controlTemperatureTask started.");
    while (true)
    {
        if (sysStat.bbqTemperature > 0)
        {
            controlTemperature(sysStat);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Tarefa para obter a temperatura calibrada do ds18b20
void getCalibratedInternalTempTask(void *parameter)
{
    _logger.logMessage("Task getCalibratedInternalTempTask started.");
    while (true)
    {
        getCalibratedInternalTemp(sysStat);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Função para criar as tarefas
void createTasks()
{
    _logger.logMessage("Creating tasks...");
    
    xTaskCreate(
        getCalibratedTempTask, // Função que será executada pela tarefa. Esta função irá obter e calibrar a temperatura.
        "TempTask",            // Nome da tarefa (útil para fins de depuração).
        4000,                  // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                  // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                     // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                   // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    _logger.logMessage("Task TempTask created.");

    xTaskCreate(
        getCalibratedTempPTask, // Função que será executada pela tarefa. Esta função irá obter e calibrar a temperatura.
        "TempPTask",            // Nome da tarefa (útil para fins de depuração).
        4000,                   // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                   // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                      // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                    // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    _logger.logMessage("Task TempPTask created.");

    xTaskCreate(
        controlTemperatureTask, // Função que será executada pela tarefa. Esta função irá controlar a temperatura.
        "ControlTempTask",      // Nome da tarefa (útil para fins de depuração).
        4000,                   // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                   // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                      // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                    // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    _logger.logMessage("Task ControlTempTask created.");

    // xTaskCreate(
    //     getCalibratedInternalTempTask, // Função que será executada pela tarefa. Esta função irá obter a temperatura interna calibrada.
    //     "ControlInternalTempTask",      // Nome da tarefa (útil para fins de depuração).
    //     2000,                   // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
    //     NULL,                   // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
    //     1,                      // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
    //     NULL                    // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    // );

    // _logger.logMessage("Task ControlInternalTempTask created.");

    _logger.logMessage("All tasks created successfully.");
}
