#ifndef TASK_HANDLER_H
#define TASK_HANDLER_H

#include "SystemStatus.h"
#include "MQTTHandler.h"

// Inicializa as tasks principais
void initializeTasks(SystemStatus& sysStat, MQTTHandler& mqtt);

// Para as tasks em execução
void stopTasks();

// Controle específico do MQTT
void startMQTTTask();
void stopMQTTTask();

#endif
