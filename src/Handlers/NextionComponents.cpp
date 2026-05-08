#include "Handlers/NextionComponents.h"

NexPage wifi       = NexPage(NEXTION_PAGE_WIFI,       0, "wifi");
NexPage welcome    = NexPage(NEXTION_PAGE_WELCOME,    0, "welcome");
NexPage menu       = NexPage(NEXTION_PAGE_MENU,       0, "menu");
NexPage monitor    = NexPage(NEXTION_PAGE_MONITOR,    0, "monitor");
NexPage BBQTemp    = NexPage(NEXTION_PAGE_BBQ_TEMP,   0, "BBQTemp");
NexPage ChunkTemp  = NexPage(NEXTION_PAGE_CHUNK_TEMP, 0, "ChunkTemp");
NexPage ap         = NexPage(NEXTION_PAGE_AP,         0, "ap");
NexPage initial    = NexPage(NEXTION_PAGE_INIT,       0, "init");

// Monitor page
NexNumber bbqTempSet   = NexNumber(NEXTION_PAGE_MONITOR, 10, "bbqTempSet");
NexNumber bbqTemp      = NexNumber(NEXTION_PAGE_MONITOR,  1, "bbqTemp");
NexNumber chunkTempSet = NexNumber(NEXTION_PAGE_MONITOR, 11, "chunkTempSet");
NexNumber chunkTemp    = NexNumber(NEXTION_PAGE_MONITOR,  2, "chunkTemp");
NexNumber bbqTempAvg   = NexNumber(NEXTION_PAGE_MONITOR,  8, "bbqTempAvg");
NexButton stopPush     = NexButton(NEXTION_PAGE_MONITOR, 18, "stop");

// BBQTemp page
NexNumber setBBQTemp     = NexNumber(NEXTION_PAGE_BBQ_TEMP, 1, "setBBQTemp");
NexNumber minBBQTemp     = NexNumber(NEXTION_PAGE_BBQ_TEMP, 6, "minBBQTemp");
NexNumber maxBBQTemp     = NexNumber(NEXTION_PAGE_BBQ_TEMP, 7, "maxBBQTemp");
NexButton setBBQTempPush = NexButton(NEXTION_PAGE_BBQ_TEMP, 8, "setBBQ");

// ChunkTemp page
NexNumber setChunkTemp     = NexNumber(NEXTION_PAGE_CHUNK_TEMP, 1, "setChunkTemp");
NexNumber minChunkTemp     = NexNumber(NEXTION_PAGE_CHUNK_TEMP, 6, "minChunkTemp");
NexNumber maxChunkTemp     = NexNumber(NEXTION_PAGE_CHUNK_TEMP, 7, "maxChunkTemp");
NexButton setChunkTempPush = NexButton(NEXTION_PAGE_CHUNK_TEMP, 8, "setChunk");

// Calibration page (page ID 6)
NexNumber caliBBQTemp    = NexNumber(NEXTION_PAGE_CALIBRATION,  4, "caliBBQTemp");
NexNumber minCaliBBQTemp = NexNumber(NEXTION_PAGE_CALIBRATION, 12, "minCaliBBQTemp");
NexNumber maxCaliBBQTemp = NexNumber(NEXTION_PAGE_CALIBRATION, 13, "maxCaliBBQTemp");
NexNumber caliChunkTemp  = NexNumber(NEXTION_PAGE_CALIBRATION,  7, "caliChunkTemp");
NexNumber minCaliChuTemp = NexNumber(NEXTION_PAGE_CALIBRATION, 14, "minCaliChuTemp");
NexNumber maxCaliChuTemp = NexNumber(NEXTION_PAGE_CALIBRATION, 15, "maxCaliChuTemp");
NexButton setCaliPush    = NexButton(NEXTION_PAGE_CALIBRATION, 10, "setCali");

NexTouch *nex_listen_list[] = {
    &setBBQTempPush,
    &setChunkTempPush,
    &stopPush,
    &setCaliPush,
    NULL
};
