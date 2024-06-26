#ifndef NEXTION_HANDLER_H
#define NEXTION_HANDLER_H

#include <Nextion.h>
#include "SystemStatus.h"

// Definição dos componentes Nextion
extern NexPage wifi;
extern NexPage welcome;
extern NexPage menu;
extern NexPage monitor;
extern NexPage BBQTemp;
extern NexPage ChunkTemp;
extern NexPage energyPg;

extern NexNumber bbqTempSet;
extern NexNumber bbqTemp;
extern NexNumber chunkTempSet;
extern NexNumber chunkTemp;
extern NexNumber bbqTempAvg;
extern NexPicture relayStatePic;

extern NexNumber setBBQTemp;
extern NexNumber minBBQTemp;
extern NexNumber maxBBQTemp;
extern NexButton setBBQTempPop;

extern NexTouch *nex_listen_list[];

// Função de inicialização do Nextion
void initNextion(SystemStatus &sysStat);
void setBBQTempPopCallback(void *ptr);

// Função para atualizar os valores das variáveis do Nextion
void updateNextionMonitorVariables(SystemStatus &sysStat);
void updateNextionSetBBQVariables(SystemStatus &sysStat);
void updateNextionSetChunkVariables(SystemStatus &sysStat);
void updateNextionEnergyVariables(SystemStatus &sysStat);
uint8_t getCurrentPageId();

#endif // NEXTION_HANDLER_H
