#ifndef NEXTION_HANDLER_H
#define NEXTION_HANDLER_H

#include "Handlers/NextionComponents.h"
#include "SystemStatus.h"
#include "Handlers/TemperatureHandler.h"

void initNextion(SystemStatus &sysStat);
uint8_t getCurrentPageId();
void setPageBackground(const char *pageName, uint32_t img_id);

void updateNextionMonitorVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetBBQVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetChunkVariables(SystemStatus &sysStat, uint8_t pageId);
void updateNextionSetCaliVariables(SystemStatus &sysStat, uint8_t pageId);

#endif // NEXTION_HANDLER_H
