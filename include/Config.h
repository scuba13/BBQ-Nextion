#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// Segurança física
// =============================================================================
#define MAX_SAFE_TEMP           280   // temperatura máxima absoluta (failsafe relé)
#define TEMP_HYSTERESIS           2   // graus de histerese do controle de temperatura

// =============================================================================
// Sensores de temperatura
// =============================================================================
#define TEMP_READ_INTERVAL      500   // ms entre leituras dos termopares (MAX6675)
#define DS18B20_ERROR_LOW    -127.0f  // valor de erro: sensor desconectado
#define DS18B20_ERROR_HIGH     85.0f  // valor de erro: power-on reset
#define THERMOCOUPLE_MAX_TEMP 500.0f  // limite superior de leitura válida

// =============================================================================
// Médias móveis
// =============================================================================
#define NUM_SAMPLES              20   // amostras para média de temperatura (sensores)
#define MOVING_AVERAGE_SIZE     180   // amostras para média de longo prazo (câmara)

// =============================================================================
// Tasks FreeRTOS — tamanhos de stack em bytes
// =============================================================================
#define TEMP_TASK_STACK        4096
#define CONTROL_TASK_STACK     2048
#define MQTT_TASK_STACK        4096
#define DIAG_TASK_STACK        2048

// =============================================================================
// Monitoramento de tasks
// =============================================================================
#define TASK_STACK_LOW_WATERMARK  512   // bytes — alerta de stack baixa
#define HEAP_LOW_WATERMARK      10000   // bytes — alerta de heap baixa
#define MAX_TASK_BLOCKED_MS     10000   // ms — timeout antes de task ser considerada travada

// =============================================================================
// Watchdog
// =============================================================================
#define WDT_TIMEOUT_SECONDS      30
#define SOFT_WDT_INTERVAL     60000   // ms — verificação do software watchdog

// =============================================================================
// Log
// =============================================================================
#define LOG_BUFFER_BYTES       1024   // tamanho do buffer de log em RAM
#define LOG_MAX_FILE_SIZE     50000   // bytes — tamanho máximo antes de rotação
#define LOG_FLUSH_INTERVAL     5000   // ms — flush periódico para LittleFS
#define LOG_METRICS_INTERVAL 300000   // ms — intervalo do relatório de diagnóstico (5 min)

// =============================================================================
// MQTT
// =============================================================================
#define MQTT_BUFFER_BYTES      1024
#define MQTT_KEEPALIVE_SEC       30
#define MQTT_RETRY_MS          5000   // ms entre tentativas de reconexão

// =============================================================================
// OTA
// =============================================================================
#define OTA_TIMEOUT_MS       300000   // ms — timeout de upload (5 minutos)
#define OTA_MIN_HEAP_BYTES    40000   // heap mínimo para iniciar update
#define OTA_MAX_SIZE       (4 * 1024 * 1024)  // 4 MB tamanho máximo do firmware
#define FIRMWARE_VERSION     "1.0.0"

// =============================================================================
// Nextion — IDs de página (confirmar contra arquivo .HMI)
// =============================================================================
#define NEXTION_PAGE_WIFI        0
#define NEXTION_PAGE_WELCOME     1
#define NEXTION_PAGE_MENU        2
#define NEXTION_PAGE_MONITOR     3
#define NEXTION_PAGE_BBQ_TEMP    4
#define NEXTION_PAGE_CHUNK_TEMP  5
#define NEXTION_PAGE_CALIBRATION 6
#define NEXTION_PAGE_AP          7
#define NEXTION_PAGE_INIT        8
#define NEXTION_PAGE_NONE     0xFF  // sentinela: nenhuma página ativa

// IDs de imagem de fundo da página Monitor (relay on/off)
#define NEXTION_BG_RELAY_ON      4
#define NEXTION_BG_RELAY_OFF     1

// =============================================================================
// Limites padrão de temperatura (usados na criação do config.json)
// =============================================================================
#define DEFAULT_MIN_BBQ_TEMP     30
#define DEFAULT_MAX_BBQ_TEMP    200
#define DEFAULT_MIN_PRT_TEMP     40
#define DEFAULT_MAX_PRT_TEMP     99
#define DEFAULT_MIN_CALI_TEMP   -20
#define DEFAULT_MAX_CALI_TEMP    20

#endif // CONFIG_H
