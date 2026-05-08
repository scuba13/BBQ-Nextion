#ifndef NEXTION_COMPONENTS_H
#define NEXTION_COMPONENTS_H

#include <Nextion.h>
#include "Config.h"

// Pages
extern NexPage wifi;
extern NexPage welcome;
extern NexPage menu;
extern NexPage monitor;
extern NexPage BBQTemp;
extern NexPage ChunkTemp;
extern NexPage ap;
extern NexPage initial;

// Monitor page
extern NexNumber bbqTempSet;
extern NexNumber bbqTemp;
extern NexNumber chunkTempSet;
extern NexNumber chunkTemp;
extern NexNumber bbqTempAvg;
extern NexButton stopPush;

// BBQTemp page
extern NexNumber setBBQTemp;
extern NexNumber minBBQTemp;
extern NexNumber maxBBQTemp;
extern NexButton setBBQTempPush;

// ChunkTemp page
extern NexNumber setChunkTemp;
extern NexNumber minChunkTemp;
extern NexNumber maxChunkTemp;
extern NexButton setChunkTempPush;

// Calibration page
extern NexNumber caliBBQTemp;
extern NexNumber minCaliBBQTemp;
extern NexNumber maxCaliBBQTemp;
extern NexNumber caliChunkTemp;
extern NexNumber minCaliChuTemp;
extern NexNumber maxCaliChuTemp;
extern NexButton setCaliPush;

extern NexTouch *nex_listen_list[];

#endif // NEXTION_COMPONENTS_H
