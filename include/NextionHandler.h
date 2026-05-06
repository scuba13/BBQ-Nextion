#ifndef NEXTION_HANDLER_H
#define NEXTION_HANDLER_H

#include <Nextion.h>
#include "SystemStatus.h"
#include "TemperatureControl.h"

// Definição dos componentes Nextion
extern NexPage wifi;
extern NexPage welcome;
extern NexPage ap;
extern NexPage initial;
extern NexTouch *nex_listen_list[];

// Função de inicialização do Nextion
void initNextion(SystemStatus &sysStat);

// Lê o ID da página atual (envia "sendme" e aguarda resposta)
uint8_t getCurrentPageId();

// Função para atualizar os valores das variáveis do Nextion
void updateNextionMonitorVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetBBQVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetChunkVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetCaliVariables(SystemStatus &sysStat, uint8_t pageId);

#endif // NEXTION_HANDLER_H
