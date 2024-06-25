#ifndef TASKHANDLER_H
#define TASKHANDLER_H

#include "SystemStatus.h"
#include "TemperatureControl.h"
#include "EnergyMonitor.h"

// Declaração das tarefas
void getCalibratedTempTask(void *parameter);
void getCalibratedTempPTask(void *parameter);

// Função para criar as tarefas
void createTasks();

#endif // TASKHANDLER_H
