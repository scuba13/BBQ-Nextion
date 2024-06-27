#include "TaskHandler.h"
#include <Arduino.h>
#include <Nextion.h>

// Declaração externa de sysStat para que as tarefas possam acessar
extern SystemStatus sysStat;
extern EnergyMonitor energyMonitor;



// Tarefa para obter a temperatura calibrada do termopar
void getCalibratedTempTask(void *parameter)
{
    while (true)
    {
        getCalibratedTemp(thermocouple, sysStat);
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}

// Tarefa para obter a temperatura calibrada do termopar de proteína
void getCalibratedTempPTask(void *parameter)
{
    while (true)
    {
        getCalibratedTempP(thermocoupleP, sysStat);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Tarefa para controlar a temperatura
void controlTemperatureTask(void *parameter)
{
    while (true)
    {
        if (sysStat.bbqTemperature > 0)
        {
            controlTemperature(sysStat);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Tarefa para monitorar a energia
void geEnergyTask(void *parameter)
{
    while (true)
    {
        energyMonitor.monitor();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Função para criar as tarefas
void createTasks()
{
    xTaskCreate(
        getCalibratedTempTask, // Função que será executada pela tarefa. Esta função irá obter e calibrar a temperatura.
        "TempTask",            // Nome da tarefa (útil para fins de depuração).
        2000,                  // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                  // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                     // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                   // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    xTaskCreate(
        getCalibratedTempPTask, // Função que será executada pela tarefa. Esta função irá obter e calibrar a temperatura.
        "TempPTask",            // Nome da tarefa (útil para fins de depuração).
        2000,                   // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                   // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                      // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                    // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    xTaskCreate(
        controlTemperatureTask, // Função que será executada pela tarefa. Esta função irá controlar a temperatura.
        "ControlTempTask",      // Nome da tarefa (útil para fins de depuração).
        2000,                   // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                   // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        1,                      // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                    // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );

    xTaskCreate(
        geEnergyTask,        // Função que será executada pela tarefa. Esta função irá controlar a temperatura.
        "EnergyMonitorTask", // Nome da tarefa (útil para fins de depuração).
        2000,                // Tamanho da pilha da tarefa. Reserva espaço para 2000 entradas.
        NULL,                // Parâmetros que são passados para a função da tarefa. No caso, nenhum parâmetro é passado.
        2,                   // Prioridade da tarefa. Aqui, a tarefa tem uma prioridade de 1.
        NULL                 // Pode armazenar o identificador da tarefa, mas não estamos armazenando aqui.
    );


}
