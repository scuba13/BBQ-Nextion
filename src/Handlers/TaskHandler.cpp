#include "TaskHandler.h"
#include <Arduino.h>

// Declaração externa de sysStat para que as tarefas possam acessar
extern SystemStatus sysStat;

// Tarefa para obter a temperatura calibrada do termopar
void getCalibratedTempTask(void *parameter)
{
    while (true)
    {
        getCalibratedTemp(thermocouple, sysStat);
        vTaskDelay(pdMS_TO_TICKS(200)); // Espera 200ms
    }
}

// Tarefa para obter a temperatura calibrada do termopar de proteína
void getCalibratedTempPTask(void *parameter)
{
    while (true)
    {
        getCalibratedTempP(thermocoupleP, sysStat);
        vTaskDelay(pdMS_TO_TICKS(200)); // Espera 200ms
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
}
