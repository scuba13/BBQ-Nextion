# Achados Exclusivos — Análise Kimi-K2.6

Itens encontrados na análise de 2026-05-06 que **não constam** no `CORRECOES.md` da branch `edu`.
Estes são bugs de concorrência, segurança de runtime e integridade de hardware ainda presentes no código.

---

## Prioridade 🔴 — Crítico (estabilidade / corrupção / segurança física)

### K-01 — `SystemStatus` sem mecanismo de sincronização
- **Arquivos:** `include/SystemStatus.h`, todos os `.cpp` que acessam `sysStat`
- **Problema:** `sysStat` é uma struct global passada por referência para ~5 tasks + callbacks do Nextion + handlers de endpoint. Nenhum campo é `volatile`, `std::atomic` ou protegido por mutex. O compilador pode reordenar acessos e o cache de cada core (ESP32-S3) pode ver valores desatualizados. Leituras/escritas de `float` e structs maiores podem sofrer *tearing*.
- **Impacto:** Temperatura alvo, estado do relé e amostras de sensor lidos de forma inconsistente entre cores. Comportamento "intermitente" impossível de reproduzir em debug.
- **Correção:** Adicionar `SemaphoreHandle_t sysStatMutex` global. Envolver **toda** leitura/escrita em `sysStat` com `xSemaphoreTake` / `xSemaphoreGive`. Para campos de leitura muito frequente (ex: `isRelayOn`), considere `std::atomic<bool>` se o toolchain suportar.

---

### K-02 — Task WDT não monitora `TempTask` nem `ControlTask`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp:10-13`, `src/Handlers/TaskHandler.cpp`
- **Problema:** `esp_task_wdt_add(NULL)` no construtor de `DiagnosticsHandler` adiciona apenas a task que estava rodando no momento (tipicamente `loop()` do Arduino, core 1). `TempTask` e `ControlTask` (core 0) nunca são registradas.
- **Impacto:** Se `temperatureTask` ou `controlTask` travarem (deadlock em SPI, sensor travado), o hardware watchdog **não** detecta. O software watchdog (`watchdogFeed()` no `loop()`) também não detecta, pois o `loop()` continua rodando normalmente enquanto as tasks críticas estão mortas. O relé pode ficar preso em ON indefinidamente.
- **Correção:** Após `xTaskCreatePinnedToCore` em `TaskHandler.cpp`, chamar `esp_task_wdt_add(tempTaskHandle)` e `esp_task_wdt_add(controlTaskHandle)`. Garantir que cada task chame `esp_task_wdt_reset()` periodicamente.

---

### K-03 — `LogHandler` buffer circular sem mutex
- **Arquivo:** `src/Handlers/LogHandler.cpp:40-62`, `141-163`, `80-95`
- **Problema:** `logHandler` é chamado concorrentemente por `TempTask`, `ControlTask`, `MQTTTask` e `loop()`. O buffer `logBuffer[1024]`, `bufferIndex`, `lastFlush`, `currentFileSize` e `fileExists` são acessados sem mutex. Duas tasks podem executar `memcpy(logBuffer + bufferIndex, ...)` simultaneamente.
- **Impacto:** Corrupção do buffer, índice inválido (`bufferIndex > LOG_BUFFER_SIZE`), escrita no LittleFS com conteúdo misturado, crash se `bufferIndex` corrompido passar para `writeLog()`.
- **Correção:** Adicionar `SemaphoreHandle_t logMutex` em `LogHandler` e envolver `logMessage`, `flushBuffer`, `writeLog`, `clearLogs` em `xSemaphoreTake` / `xSemaphoreGive`.

---

### K-04 — `Update.end(true)` aceita firmware incompleto
- **Arquivo:** `src/Handlers/OTAHandler.cpp:95-114`
- **Problema:** `endUpdate()` chama `Update.end(true)`. O parâmetro `true` (*evenIfRemaining*) faz o ESP32 aceitar o firmware **mesmo que nem todos os bytes tenham sido escritos**.
- **Impacto:** Upload truncado ou interrompido gera boot com firmware corrompido. Possível brick ou loop de boot.
- **Correção:** Usar `Update.end(false)`. Tratar retorno `false` como erro de firmware incompleto.

---

## Prioridade 🟠 — Alto (dados incorretos ou degradação)

### K-05 — DS18B20 sem validação de valores de erro
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp:36-43`
- **Problema:** `sensors.getTempCByIndex(0)` retorna `DEVICE_DISCONNECTED_C` (-127.0) se o sensor estiver desconectado, ou `85.0` em power-on reset. O código converte diretamente para `int` via `round(temp)`, propagando o erro como temperatura real.
- **Impacto:** Temperatura interna do ESP exibida como -127°C ou 85°C. Se usada para compensação de leitura ou lógica de corte, comportamento errático.
- **Correção:** Verificar `if (temp == DEVICE_DISCONNECTED_C || temp == 85.0f) { return; }` antes de atribuir. Manter última leitura válida.

---

### K-06 — MAX6675 sem validação de leitura inválida
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp:46-74`
- **Problema:** Bibliotecas MAX6675 comumente retornam `NAN` ou `0` em falha de SPI/termopar aberto. O código soma calibração e computa média sem checar `isnan()` ou limites físicos.
- **Impacto:** Média móvel de temperatura corrompida com valores `0` ou `NAN`. Relé pode ligar/desligar em momentos errados.
- **Correção:** Validar com `isnan(temp) || temp < 0 || temp > 500` antes de inserir na amostra. Descartar leituras inválidas.

---

### K-07 — `neopixelWrite` chamado concorrentemente sem sincronização
- **Arquivos:** `src/Handlers/TemperatureControlHandler.cpp:106,112,132,137`, `src/main.cpp:35`, `src/Handlers/WiFiHandler.cpp:15`
- **Problema:** `neopixelWrite()` usa o periférico RMT do ESP32-S3. Chamá-la de `loop()`, `TempTask`, `ControlTask` e callbacks WiFi sem coordenação pode causar glitch visual ou, em versões antigas do core, corrupção do estado do driver RMT.
- **Impacto:** LED RGB piscando erraticamente; em casos extremos, crash do driver RMT.
- **Correção:** Centralizar o controle do LED em uma única função chamada apenas do `loop()`, ou proteger com `portMUX_TYPE` se necessário.

---

## Prioridade 🟡 — Médio (performance / manutenção)

### K-08 — `String` em hot paths causando fragmentação de heap
- **Arquivos:** `src/Handlers/MQTTHandler.cpp`, `src/Handlers/LogHandler.cpp`
- **Problema:** Concatenações de `String` em `publishAllMessages()` (`String clientId = ...`, `String topic = ...`) e em `logMessage()` (`String timeStamp = ...`) alocam no heap a cada ciclo. Em operação prolongada (dias), o heap do ESP32 fragmenta. Mesmo com "heap livre" aparente, uma alocação pode falhar.
- **Impacto:** Falha silenciosa de alocação → MQTT não publica, log não grava, ou crash por `std::bad_alloc` / `panic`.
- **Correção:** Substituir por `snprintf` em buffers `char[]` fixos (stack ou memória estática) nos hot paths.

---

### K-09 — Divisão por zero no cálculo de fragmentação de heap
- **Arquivo:** `src/Handlers/DiagnosticsHandler.cpp:122-123`
- **Problema:** `100 - ((float)metrics.maxAllocHeap * 100) / metrics.freeHeap`. Se `metrics.freeHeap` for `0` (OOM iminente), ocorre divisão por zero. Em floating-point não crasha com trap, mas gera `NaN`/`Inf` que pode propagar.
- **Impacto:** Métrica de saúde do sistema retorna valor inválido quando mais importa (memória esgotada).
- **Correção:** Proteger com `if (metrics.freeHeap == 0) return 100;`.

---

### K-10 — `LogHandler` usa `dbSerial` definido por macro de biblioteca de terceiro
- **Arquivo:** `src/Handlers/LogHandler.cpp`
- **Problema:** `dbSerial` vem de `#include <Nextion.h>` → `NexConfig.h`, que define `#define dbSerial Serial`. Se `DEBUG_SERIAL_ENABLE` for desabilitado no `NexConfig.h` (algo natural para produção), todos os logs do sistema param silenciosamente.
- **Impacto:** Sem logs de diagnóstico em campo, debug de crashes remoto fica impossível.
- **Correção:** Substituir `dbSerial` por `Serial` diretamente no `LogHandler`. Não depender de macro de lib externa para logging do sistema.

---

## Resumo de Overlap com `CORRECOES.md`

| Nosso ID | ID no CORRECOES.md | Nota |
|---|---|---|
| K-05 | C-08 (parcial) | CORRECOES foca em partição OTA errada; não menciona `Update.end(true)` |
| K-06 | — | Não consta no CORRECOES.md |
| K-09 | C-06 (parcial) | CORRECOES foca em unidade (% vs bytes); não menciona divisão por zero |
| K-10 | C-35 (parcial) | CORRECOES foca em includes desnecessários; não menciona fragilidade do `dbSerial` |

Todos os demais itens (K-01 a K-04, K-07, K-08) são **exclusivos** desta análise.

---

*Documento gerado em 2026-05-06. Estado de referência: branch `melhorias` (pós-Sprint 1).*