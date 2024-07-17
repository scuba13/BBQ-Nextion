#ifndef NEXTION_HANDLER_H
#define NEXTION_HANDLER_H

#include <Nextion.h>
#include "SystemStatus.h"
#include "TemperatureControl.h"

// Definição dos componentes Nextion
extern NexPage wifi;
extern NexPage welcome;
extern NexTouch *nex_listen_list[];

// Função de inicialização do Nextion
void initNextion(SystemStatus &sysStat);

// Função para atualizar os valores das variáveis do Nextion
void updateNextionMonitorVariables(SystemStatus &sysStat);
void updateNextionSetBBQVariables(SystemStatus &sysStat);
void updateNextionSetChunkVariables(SystemStatus &sysStat);
void updateNextionSetCaliVariables(SystemStatus &sysStat);

#endif // NEXTION_HANDLER_H
