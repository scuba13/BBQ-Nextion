# Análise de Qualidade — BBQ-Nextion

Gerado em: 2026-05-06  
Estado de referência: branch `melhorias` (pós-Sprint K)

---

## Como usar este documento

Cada item tem um **ID único** (`Q-XX`), prioridade, localização e o que precisa ser analisado/feito.  
Ao iniciar uma análise, marque como `[ em andamento ]`. Ao concluir, marque como `[x]`.

---

## Prioridade 1 — Segurança Física

*Itens que podem causar dano físico ao equipamento ou ao usuário.*

---

### Q-01 — Failsafe de temperatura máxima absoluta
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp`
- **Problema:** Não existe um teto absoluto de temperatura no firmware. Se o sensor MAX6675 retornar um valor alto (ex: 450°C — válido pela validação do K-06), o relé permanece ligado até atingir esse valor. O setpoint configurado pelo usuário pode ser ignorado por bug ou por mensagem MQTT malformada.
- **Risco:** Overheating do equipamento, incêndio.
- **Análise a fazer:**
  - Definir `MAX_SAFE_TEMP` (ex: 280°C para um pit smoker) como constante em `Config.h` (E-10)
  - Em `controlTemperature()`, adicionar desligamento forçado do relé se `calibratedTemp > MAX_SAFE_TEMP`, independente do setpoint
  - Logar evento de emergência e notificar via MQTT
- **Correção sugerida:**
  ```cpp
  const int MAX_SAFE_TEMP = 280;
  if (sysStat.calibratedTemp > MAX_SAFE_TEMP) {
      digitalWrite(RELAY_PIN, LOW);
      sysStat.isRelayOn = false;
      logHandler.logError("EMERGÊNCIA: temperatura acima do limite seguro!");
      return;
  }
  ```

---

### Q-02 — Estado do GPIO no boot antes do fastInit()
- **Status:** `[ ]`
- **Arquivo:** `src/main.cpp:34`
- **Problema:** No ESP32-S3, GPIOs bootam como input/floating até que o firmware os configure. Entre o boot e a execução de `fastInit()`, o pino `RELAY_PIN` (GPIO 1) pode estar em estado indefinido por milissegundos. Se houver ruído elétrico ou o relé tiver memória de estado, pode ligar momentaneamente.
- **Risco:** Relé pulsando na inicialização.
- **Análise a fazer:**
  - Verificar se é possível usar pull-down externo no RELAY_PIN (solução de hardware)
  - Ou garantir que `pinMode + digitalWrite(RELAY_PIN, LOW)` sejam as primeiras instruções executadas, antes de qualquer outra inicialização
  - Medir com osciloscópio ou multímetro se o relé pulsa durante o boot

---

### Q-03 — Comportamento da proteína quando atinge temperatura alvo
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp`
- **Problema:** Não há lógica de "proteína atingiu temperatura alvo" que desligue o relé ou emita alarme. O controle atual gerencia apenas a temperatura da câmara (BBQ). A temperatura da proteína é monitorada e exibida, mas não dispara nenhuma ação automática.
- **Análise a fazer:**
  - Definir o comportamento esperado: quando `calibratedTempP >= proteinTemperature`, o sistema deve: (a) apenas alertar via MQTT/Nextion? (b) desligar o relé? (c) mudar para modo de manutenção de temperatura?
  - Implementar a lógica após decisão de negócio

---

## Prioridade 2 — Confiabilidade de Longa Duração

*Itens que fazem diferença entre rodar por horas vs. rodar por dias sem problema.*

---

### Q-04 — Monitorar heap e stack após 24h+ de operação
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp`, `/api/v1/diagnostics`
- **Problema:** Fragmentação de heap e stack overflow silencioso só aparecem após horas de operação contínua. O endpoint `/api/v1/diagnostics` já expõe as métricas, mas nunca foram observadas após operação prolongada.
- **Análise a fazer:**
  - Deixar o dispositivo rodando por 24h+ em condições normais de uso
  - Amostrar `GET /api/v1/diagnostics` a cada hora e registrar:
    - `heap.free` — deve ser estável, não cair progressivamente
    - `heap.fragmentation` — abaixo de 70% é saudável
    - `stack.free` — deve ser > 512 bytes em cada task
  - Se `heap.min` cair progressivamente → investigar leak de String/JSON
  - Se `stack.free` < 512 → aumentar stack da task correspondente em `TaskHandler.cpp`
- **Thresholds de alerta (já configurados):**
  - `freeHeap < 10.000 bytes` → sistema não saudável
  - `heapFragmentation > 70%` → sistema não saudável
  - `freeStack < 256 bytes` → sistema não saudável

---

### Q-05 — Teste de reconexão WiFi e MQTT após perda de rede
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/WiFiHandler.cpp`, `src/Handlers/MQTTHandler.cpp`
- **Problema:** Não foi testado o comportamento quando o roteador reinicia ou perde sinal com o dispositivo ligado. O WiFiManager tem timeout de 120s para o portal AP — após esse tempo, o comportamento não está documentado.
- **Análise a fazer:**
  - Teste 1: Desligar o roteador com o dispositivo ligado. Aguardar 5 minutos. Religar o roteador. O dispositivo deve reconectar automaticamente sem intervenção.
  - Teste 2: Reiniciar o roteador (desligar e ligar em 30s). Verificar se o controle de temperatura continuou funcionando durante a ausência de WiFi.
  - Teste 3: Desligar o broker MQTT com WiFi ativo. Verificar se `MQTTHandler::connect()` tenta reconectar a cada 5s (MQTT_RETRY_INTERVAL) sem travar.
  - Verificar o que acontece após o portal AP abrir e fechar sem configuração — o ESP32 entra em loop de reboot?

---

### Q-06 — Registrar causa do reboot com `esp_reset_reason()`
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/FileSystemHandler.cpp` (função initializeAndLoadConfig)
- **Problema:** O log registra `=== BOOT ===` mas não a causa do reboot. Em campo, é impossível saber se o dispositivo reiniciou por watchdog, panic, power-off ou software. Isso é crítico para debugging remoto.
- **Correção sugerida:**
  ```cpp
  // Em initializeAndLoadConfig(), após "=== BOOT ==="
  esp_reset_reason_t reason = esp_reset_reason();
  const char* reasons[] = {
      "desconhecido", "power-on", "reset externo", "software",
      "panic/exception", "watchdog int.", "watchdog task",
      "watchdog outros", "sleep profundo", "brownout", "SDIO"
  };
  logHandler.logMessage("Causa do reboot: " + String(reasons[min((int)reason, 10)]));
  ```

---

### Q-07 — Contadores de eventos de falha no DiagnosticsHandler
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/DiagnosticsHandler.h`, `src/Handlers/DiagnosticsHandler.cpp`
- **Problema:** O diagnóstico atual reporta métricas instantâneas (heap livre agora, stack agora). Não há contadores acumulados de falhas que permitam ver a saúde do sistema ao longo do tempo.
- **Métricas a adicionar:**
  ```cpp
  struct HealthCounters {
      uint32_t wifiReconnections = 0;
      uint32_t mqttReconnections = 0;
      uint32_t sensorReadErrors = 0;   // leituras rejeitadas pelo K-05/K-06
      uint32_t otaAttempts = 0;
      uint32_t otaFailures = 0;
      uint32_t watchdogFeeds = 0;
  };
  ```
- **Expor em** `/api/v1/diagnostics` e no log periódico do `DiagTask`

---

## Prioridade 3 — Observabilidade em Campo

*Itens que facilitam debugging remoto sem ter acesso físico ao dispositivo.*

---

### Q-08 — Timestamps reais via NTP nos logs
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/LogHandler.cpp`
- **Problema:** Todos os logs têm timestamp em segundos desde o boot (`millis()/1000`). Após 10 dias de uptime, o log mostra `864000s: [ERROR] ...` — inútil para correlacionar eventos com horário real. O WiFi já está disponível quando os logs começam a ser gerados.
- **Análise a fazer:**
  - Adicionar `configTime(timezone_offset, 0, "pool.ntp.org")` após conectar WiFi
  - Em `writeLog()`, usar `time()` para obter timestamp Unix e formatar como `HH:MM:SS DD/MM`
  - Fallback: se NTP não disponível, usar `millis()` como atualmente
- **Impacto:** Melhora drasticamente a utilidade do log para debugging em produção

---

### Q-09 — Expansão do endpoint `/api/v1/diagnostics`
- **Status:** `[ ]`
- **Arquivo:** `src/Endpoints/DiagnosticsEndpoints.cpp`
- **Problema:** O endpoint atual expõe heap, CPU, stack e uptime. Para debugging real, faltam informações de estado do sistema.
- **Campos a adicionar:**
  ```json
  {
    "system": {
      "resetReason": "watchdog task",
      "uptime": 86400,
      "healthy": true
    },
    "wifi": {
      "connected": true,
      "rssi": -65,
      "reconnections": 2
    },
    "mqtt": {
      "connected": true,
      "reconnections": 1
    },
    "sensors": {
      "bbqReadErrors": 0,
      "proteinReadErrors": 0,
      "internalReadErrors": 0
    },
    "tasks": {
      "tempTaskStack": 1024,
      "controlTaskStack": 512,
      "mqttTaskStack": 2048
    }
  }
  ```

---

### Q-10 — Tela de status no Nextion durante boot e erros
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/NextionHandler.cpp`, `.HMI` do Nextion
- **Problema:** Durante o boot, o usuário vê a tela de inicialização mas não recebe feedback sobre o estado (WiFi conectando, MQTT conectando, config carregando). Se algo falhar, a tela simplesmente trava na última tela exibida.
- **Análise a fazer:**
  - Adicionar mensagens de status na tela `initial` (page 8) durante boot
  - Exibir mensagem de erro se WiFi falhar após timeout
  - Exibir indicador de "sem WiFi" ou "sem MQTT" na página de monitor quando desconectado

---

## Prioridade 4 — Estrutura do Código

*Itens da Sprint 6 que melhoram manutenibilidade a longo prazo.*

---

### Q-11 — Separar DeviceConfig de RuntimeState em SystemStatus
- **Status:** `[ ]`
- **Arquivo:** `include/SystemStatus.h`
- **Problema:** `SystemStatus` mistura configuração persistida com estado de runtime. Isso é a raiz de bugs como C-33 (kWhCost não salvo) — qualquer campo novo precisa ser adicionado manualmente ao save/load.
- **Proposta:**
  ```cpp
  struct DeviceConfig {      // Tudo que vai para config.json
      int bbqSetpoint;
      int proteinSetpoint;
      char mqttServer[100];
      char apiKey[33];
      // ...
  };

  struct RuntimeState {      // Nunca persistido
      int calibratedTemp;
      float tempSamples[NUM_SAMPLES];
      bool isRelayOn;
      // ...
  };

  struct SystemStatus {
      DeviceConfig config;
      RuntimeState state;
  };
  ```
- **Impacto:** `saveConfigToFile()` passa a serializar `config` inteiro — zero risco de esquecer campo.
- **Nota:** Esta é a E-07 da Sprint 6.

---

### Q-12 — Criar `Config.h` com todas as constantes do sistema
- **Status:** `[ ]`
- **Problema:** Constantes espalhadas por 7 arquivos diferentes. Valores mágicos como `30`, `250`, `3` (page ID), `2` (hysteresis) sem contexto.
- **Constantes a centralizar:**
  ```cpp
  // Temperatura
  #define MAX_SAFE_TEMP        280   // failsafe absoluto
  #define TEMP_HYSTERESIS        2   // graus de histerese do relé

  // Tasks
  #define TEMP_TASK_STACK     4096
  #define CONTROL_TASK_STACK  2048
  #define MQTT_TASK_STACK     4096

  // Limites padrão
  #define DEFAULT_MIN_BBQ_TEMP  30
  #define DEFAULT_MAX_BBQ_TEMP 200

  // Nextion page IDs
  #define NEXTION_PAGE_WIFI      0
  #define NEXTION_PAGE_WELCOME   1
  #define NEXTION_PAGE_MENU      2
  #define NEXTION_PAGE_MONITOR   3
  #define NEXTION_PAGE_BBQ_TEMP  4
  #define NEXTION_PAGE_CHUNK     5
  #define NEXTION_PAGE_CALI      6
  #define NEXTION_PAGE_AP        7
  #define NEXTION_PAGE_INIT      8
  ```
- **Nota:** Esta é a E-10 da Sprint 6.

---

### Q-13 — Verificar page ID real da calibração no Nextion (C-30)
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/NextionHandler.cpp`
- **Problema:** C-30 foi deixado em aberto na Sprint 1. A página de calibração usa page ID 6 no código, mas a `energyPg` (agora removida) também usava 6. Após a remoção da energy page (Sprint 4), o conflict foi parcialmente resolvido, mas o page ID 6 para calibração precisa ser confirmado contra o arquivo `.HMI` do Nextion.
- **Análise a fazer:**
  - Abrir o arquivo `.HMI` no Nextion Editor e verificar qual é o page ID real da tela de calibração
  - Se for diferente de 6, atualizar `updateNextionSetCaliVariables()` e os componentes `NexNumber` de calibração
  - Testar fisicamente que os componentes de calibração respondem na tela correta

---

## Prioridade 5 — Testabilidade

*Itens que facilitam verificar que o código funciona corretamente sem hardware.*

---

### Q-14 — Suite de testes de integração via API
- **Status:** `[ ]`
- **Problema:** Não há nenhuma forma automatizada de testar o firmware. Cada mudança requer upload e teste manual.
- **Proposta:** Criar um script Python/Node que, dado o IP do dispositivo, executa uma bateria de testes via HTTP:
  ```
  test_01: GET /api/v1/monitor → status 200, campos obrigatórios presentes
  test_02: PATCH /api/v1/temperature/config com valor inválido → 400
  test_03: PATCH /api/v1/temperature/config com valor válido → 200, valor confirmado no GET
  test_04: PATCH /api/v1/auth/config → set key → GET sem key → 401
  test_05: GET /api/v1/diagnostics → healthy=true
  test_06: GET /api/v1/log/content → logContent não vazio
  ```
- **Ferramenta:** `pytest` + `requests` ou Postman collection

---

### Q-15 — Abstrair sensores para permitir valores simulados
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp`
- **Problema:** `thermocouple.readCelsius()` é uma chamada direta ao hardware. Impossível testar a lógica de controle sem o sensor físico conectado.
- **Proposta mínima:** Adicionar endpoint de desenvolvimento `POST /api/v1/debug/inject-temp` que force valores de `calibratedTemp` e `calibratedTempP` por N segundos. Permite testar comportamento do relé, display Nextion e publicação MQTT sem o sensor.
- **Nota:** Apenas em builds de debug (`#ifdef DEBUG`).

---

## Prioridade 6 — OTA e Atualização

*Robustez do processo de atualização de firmware em campo.*

---

### Q-16 — Verificação de MD5 no OTA
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/OTAHandler.cpp`
- **Problema:** `_verifyPartition()` calcula o MD5 do firmware gravado mas não compara com um hash esperado. A verificação real seria:
  1. Cliente envia header `X-Firmware-MD5: <hash>` junto com o arquivo
  2. Após gravar, calcular MD5 da partição
  3. Comparar os dois — se divergir, abortar e fazer rollback
- **Impacto:** Detecta corrupção durante a transferência (rede instável)

---

### Q-17 — Tabela de partições customizada
- **Status:** `[ ]`
- **Arquivo:** `platformio.ini`, `boards/`
- **Problema:** O projeto usa a tabela de partições padrão do Arduino para ESP32-S3. Para o flash de 16MB (N16R8), a tabela padrão pode não distribuir o espaço de forma otimizada. Verificar se há espaço adequado para:
  - App OTA 0 + App OTA 1 (duas partições de firmware)
  - LittleFS (web UI + logs + config)
- **Análise a fazer:**
  - Verificar com `esptool.py --chip esp32s3 read_flash_status` ou via PlatformIO verbose build qual tabela está sendo usada
  - Comparar o tamanho atual do firmware (1MB) com o espaço disponível por partição OTA

---

## Prioridade 7 — Desgaste do Flash

*Itens que afetam a vida útil do dispositivo em operação prolongada.*

---

### Q-18 — Frequência de escrita do config.json
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/FileSystemHandler.cpp`
- **Problema:** `saveConfigToFile()` é chamado a cada mudança de configuração — inclusive quando o usuário ajusta o setpoint pelo Nextion (uso frequente). Cada chamada reescreve o arquivo inteiro (~1KB). O flash do ESP32 tem ~100.000 ciclos de escrita por setor.
- **Análise a fazer:**
  - Adicionar debounce: só salvar 5s após a última mudança, em vez de salvar imediatamente
  - Implementar o backup para `/config.bak.json` que foi declarado mas nunca implementado
  - Monitorar com contador em `DiagnosticsHandler` quantas vezes o config é salvo por hora

---

### Q-19 — Tamanho e rotação do log
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/LogHandler.h` (MAX_LOG_SIZE = 50KB), `src/Handlers/DiagnosticsHandler.cpp`
- **Problema:** O `DiagTask` loga ~7 linhas a cada 60s (relatório de diagnóstico). Com as mensagens normais de operação (MQTT, temperatura, etc.), o log pode encher em poucas horas.
- **Análise a fazer:**
  - Medir quanto tempo leva para o log atingir 50KB em operação normal
  - Considerar aumentar `METRICS_INTERVAL` para 300s (5 minutos) no DiagTask
  - Considerar reduzir o conteúdo do relatório de diagnóstico para uma única linha

---

## Rastreabilidade

| ID | Área | Prioridade | Status |
|---|---|---|---|
| Q-01 | Segurança Física | 🔴 Crítico | `[x]` |
| Q-02 | Segurança Física | 🔴 Crítico | `[x]` |
| Q-03 | Segurança Física | 🟠 Alto | `[ ]` |
| Q-04 | Confiabilidade | 🔴 Crítico | `[ ]` |
| Q-05 | Confiabilidade | 🔴 Crítico | `[ ]` |
| Q-06 | Confiabilidade | 🟠 Alto | `[x]` |
| Q-07 | Confiabilidade | 🟠 Alto | `[x]` |
| Q-08 | Observabilidade | 🟠 Alto | `[ ]` |
| Q-09 | Observabilidade | 🟡 Médio | `[ ]` |
| Q-10 | Observabilidade | 🟡 Médio | `[ ]` |
| Q-11 | Estrutura | 🟡 Médio | `[ ]` |
| Q-12 | Estrutura | 🟡 Médio | `[ ]` |
| Q-13 | Estrutura | 🟠 Alto | `[ ]` |
| Q-14 | Testabilidade | 🟡 Médio | `[ ]` |
| Q-15 | Testabilidade | 🟡 Médio | `[ ]` |
| Q-16 | OTA | 🟠 Alto | `[ ]` |
| Q-17 | OTA | 🟡 Médio | `[ ]` |
| Q-18 | Desgaste Flash | 🟡 Médio | `[ ]` |
| Q-19 | Desgaste Flash | 🟡 Médio | `[ ]` |

---

## Ordem de execução sugerida

```
Fase 1 — Segurança física (pode ser feito antes da Sprint 6)
  Q-01  failsafe de temperatura máxima absoluta
  Q-02  verificar/garantir estado do GPIO no boot
  Q-03  definir comportamento quando proteína atinge setpoint

Fase 2 — Confiabilidade (requer hardware rodando por horas)
  Q-04  monitorar heap/stack após 24h de operação
  Q-05  testar reconexão WiFi e MQTT em campo
  Q-06  registrar causa do reboot
  Q-07  adicionar contadores de falha ao DiagnosticsHandler

Fase 3 — Observabilidade (melhora debugging em campo)
  Q-08  timestamps reais via NTP
  Q-09  expandir /api/v1/diagnostics
  Q-10  feedback de status no Nextion durante boot

Fase 4 — Estrutura (junto com Sprint 6)
  Q-11  separar DeviceConfig de RuntimeState (E-07)
  Q-12  criar Config.h com constantes (E-10)
  Q-13  confirmar page ID de calibração no Nextion (C-30)

Fase 5 — Testabilidade e OTA
  Q-14  suite de testes de integração via API
  Q-15  endpoint de injeção de temperatura para debug
  Q-16  verificação de MD5 no OTA
  Q-17  revisar tabela de partições

Fase 6 — Otimização de longo prazo
  Q-18  debounce no save do config.json + backup
  Q-19  ajuste do intervalo e tamanho do log
```
