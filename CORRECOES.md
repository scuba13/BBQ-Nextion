# Plano de Correções — BBQ-Nextion

Gerado em: 2026-05-06  
Branch de referência: `edu`

---

## Como usar este documento

Cada item tem um **ID único** (`C-XX`), status, localização exata e a correção esperada.  
Ao iniciar uma correção, marque como `[ em andamento ]`. Ao concluir, marque como `[x]`.

---

## Prioridade 🔴 — Crítico (quebra funcionalidade)

### C-01 — Duplicação de tasks com race condition
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp`, `src/Handlers/TaskHandler.cpp`
- **Problema:** `setup()` chama `initializeTasks()` e depois `createTasks()`. As duas funções criam tasks concorrentes que leem e escrevem os mesmos campos de `sysStat` sem mutex. Resultado: `sysStat.tempSamples`, `nextSampleIndex`, `calibratedTemp`, `calibratedTempP` sofrem race condition entre cores. O `controlTemperature()` pode ser chamado de dois cores simultaneamente, acionando `digitalWrite(RELAY_PIN)` de forma imprevisível.
- **Tasks duplicadas:**
  - `TempTask` (de `initializeTasks`) faz o mesmo que `getCalibratedTempTask` + `getCalibratedTempPTask` (de `createTasks`)
  - `ControlTask` (de `initializeTasks`) faz o mesmo que `controlTemperatureTask` (de `createTasks`)
- **Correção:** Remover a chamada a `createTasks()` de `main.cpp::setup()`. `initializeTasks()` já cobre todos os cenários. Verificar se `getCalibratedInternalTempTask` (atualmente comentada em `createTasks`) precisa ser mantida — se sim, migrar para dentro de `initializeTasks`.

---

### C-02 — `MQTTHandler::begin()` nunca é chamado
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp`, `src/Handlers/MQTTHandler.cpp`, `src/Endpoints/MQTTConfigEndpoints.cpp`
- **Problema:** `begin(server, port, user, pass)` é o único lugar que chama `client.setServer()` e popula `credentials`. Sem essa chamada, toda tentativa de conexão MQTT falha silenciosamente (`client.setServer` nunca executado). Além disso, o endpoint PATCH `/api/v1/mqtt/config` salva a config no filesystem mas não reconfigura o cliente em runtime.
- **Correção:**
  1. Em `main.cpp::setup()`, após `fileSystem.initializeAndLoadConfig(...)`, chamar:
     ```cpp
     if (sysStat.isHAAvailable) {
         mqttHandler.begin(sysStat.mqttServer, sysStat.mqttPort,
                           sysStat.mqttUser, sysStat.mqttPassword);
     }
     ```
  2. Em `MQTTConfigEndpoints.cpp` (PATCH handler), após `fileSystem.saveConfigToFile(systemStatus)`, obter referência ao `mqttHandler` e chamar `begin()` + `startMQTTTask()` / `stopMQTTTask()` conforme `isHAAvailable`. Ver também **C-10**.

---

### C-03 — `processMessage()` declarado mas não implementado
- **Status:** `[ ]`
- **Arquivos:** `include/MQTTHandler.h`, `src/Handlers/MQTTHandler.cpp`
- **Problema:** `handleCallback()` chama `processMessage(topic, payload)`. O método é declarado como `private` no header mas não tem implementação no `.cpp`. Quando C-02 for corrigido e `begin()` passar a ser chamado, o lambda de callback será registrado e o linker quebra.
- **Correção:** Implementar `processMessage()` delegando para `messageHandler()`:
  ```cpp
  void MQTTHandler::processMessage(const String& topic, const String& payload) {
      // converte para char* para compatibilidade
      char topicBuf[topic.length() + 1];
      char payloadBuf[payload.length() + 1];
      topic.toCharArray(topicBuf, sizeof(topicBuf));
      payload.toCharArray(payloadBuf, sizeof(payloadBuf));
      messageHandler(topicBuf, (byte*)payloadBuf, payload.length());
  }
  ```
  Avaliar se os tópicos subscritos (`bbq/command`, `bbq/config`) em `subscribeToTopics()` batem com os esperados em `messageHandler()` (`sensor/bbq_set_temperature/set`, `sensor/protein_temperature/set`, `func/reset_cmd`) — provavelmente não batem e precisam ser alinhados.

---

### C-04 — `initWiFi()` nunca chamado — sem fallback AP para primeiro uso
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp`, `src/Handlers/WiFiHandler.cpp`
- **Problema:** `fastInit()` chama apenas `WiFi.begin()` (sem argumentos), que usa credenciais NVS. Em dispositivo sem credenciais salvas, o WiFi não conecta e o portal AP nunca é aberto. `initWiFi()` com WiFiManager existe mas não é chamada.
- **Correção:** Em `setup()`, após `fastInit()`, chamar `initWiFi(sysStat, logHandler)`. Avaliar se `fastInit()` deve manter o `WiFi.begin()` para tentativa rápida antes do WiFiManager timeout, ou se delega tudo para `initWiFi()`.

---

## Prioridade 🟠 — Bugs de Lógica

### C-05 — Handle de task deletada permanece no vetor de monitoramento
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp:49–76`
- **Problema:** `handleTaskTimeout()` chama `vTaskDelete(task.handle)` mas a `TaskInfo` continua em `monitoredTasks`. Na próxima iteração de `checkTasks()`, `eTaskGetState()` é chamado com handle inválido → comportamento indefinido / crash.
- **Correção:** Em `checkTasks()`, após detectar timeout e chamar `handleTaskTimeout()`, remover a entrada do vetor:
  ```cpp
  for (auto it = monitoredTasks.begin(); it != monitoredTasks.end(); ) {
      if (eTaskGetState(it->handle) == eBlocked &&
          now - it->lastActiveTime > MAX_TASK_BLOCKED_TIME) {
          handleTaskTimeout(*it);
          it = monitoredTasks.erase(it);
      } else {
          if (eTaskGetState(it->handle) != eBlocked) it->lastActiveTime = now;
          ++it;
      }
  }
  ```

---

### C-06 — `isHealthy()` compara bytes com threshold percentual
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp:138`
- **Problema:** `metrics.freeStack = uxTaskGetStackHighWaterMark(NULL)` retorna bytes disponíveis no stack da task atual (high water mark). O comentário diz "20%" mas o threshold `> 20` é irrisório (20 bytes).
- **Correção:** Definir threshold em bytes condizente com os stacks configurados (`TEMP_TASK_STACK = 3072`, `CONTROL_TASK_STACK = 2048`). Exemplo: `bool stackOk = metrics.freeStack > 256;` — ou calcular a percentagem a partir do tamanho total do stack, o que requer passar o tamanho total como parâmetro. Corrigir também o comentário.

---

### C-07 — `cureProcessMode` ativado mas nunca verificado
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/SystemEndpoints.cpp:13`, `src/Handlers/TemperatureControlHandler.cpp`
- **Problema:** `POST /api/v1/system/activateCure` seta `cureProcessMode = true`, mas nenhuma parte do `controlTemperature()` ou de qualquer task verifica esse flag. A feature é um stub sem efeito.
- **Correção (opção A):** Implementar a lógica de cura (ex: perfil de temperatura por estágios usando `CureState`).
- **Correção (opção B):** Remover o endpoint e a flag enquanto não implementado, para não gerar confusão.

---

### C-08 — `OTAHandler::verifyFirmware()` verifica a partição errada
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/OTAHandler.cpp:144`
- **Problema:** Após `Update.end()`, o novo firmware foi gravado na partição de update, mas o sistema ainda executa da partição anterior. `esp_ota_get_running_partition()` retorna a partição **atual** (antiga). A verificação de `ESP_IMAGE_HEADER_MAGIC` confirma o firmware antigo, não o novo.
- **Correção:** Verificar a próxima partição de boot (onde o novo firmware foi gravado):
  ```cpp
  bool OTAHandler::verifyFirmware() {
      const esp_partition_t* nextBoot = esp_ota_get_next_update_partition(NULL);
      if (!nextBoot) { ... return false; }
      uint32_t magicNumber;
      esp_partition_read(nextBoot, 0, &magicNumber, sizeof(magicNumber));
      if (magicNumber != ESP_IMAGE_HEADER_MAGIC) { ... return false; }
      return _verifyPartition(nextBoot);
  }
  ```

---

### C-09 — Histerese de temperatura assimétrica
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/TemperatureControlHandler.cpp:130`
- **Problema:** O relay liga em `setpoint - 2°C` e desliga em `setpoint`. O ponto de equilíbrio fica sempre 0–2°C abaixo do setpoint desejado. O valor de histerese é hardcoded e não configurável.
- **Correção sugerida:** Centralizar a histerese em torno do setpoint:
  ```cpp
  const int HYSTERESIS = 2;
  if (temp <= sysStat.bbqTemperature - HYSTERESIS) {
      // relay ON
  } else if (temp >= sysStat.bbqTemperature + HYSTERESIS) {
      // relay OFF
  }
  ```
  E/ou tornar `HYSTERESIS` configurável via API e persistido no config.

---

### C-10 — MQTT task não inicia/para quando `isHAAvailable` muda
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/MQTTConfigEndpoints.cpp:92`, `src/Handlers/TaskHandler.cpp`
- **Problema:** O endpoint PATCH de `/api/v1/mqtt/config` salva a config e atualiza `sysStat.isHAAvailable`, mas não chama `startMQTTTask()` / `stopMQTTTask()`. A mudança só tem efeito após reboot.
- **Correção:** No endpoint PATCH, após salvar, verificar o delta de `isHAAvailable` e agir:
  ```cpp
  bool wasAvailable = systemStatus.isHAAvailable; // lê antes de atualizar
  // ... atualiza systemStatus ...
  if (!wasAvailable && systemStatus.isHAAvailable) {
      mqttHandler.begin(...);
      startMQTTTask();
  } else if (wasAvailable && !systemStatus.isHAAvailable) {
      stopMQTTTask();
  }
  ```
  Requer que o endpoint receba referência ao `mqttHandler` — ajustar assinatura de `registerMQTTConfigEndpoints`.

---

### C-11 — Código morto: `updateRelayState()`, `pidControl`, `CureState`, campos não usados
- **Status:** `[ ]`
- **Arquivos:** `include/TemperatureControl.h`, `src/Handlers/TemperatureControlHandler.cpp`, `include/SystemStatus.h`
- **Itens para remover:**
  - `updateRelayState()` — declarada, implementada, nunca chamada
  - `pidControl` struct com `Kp/Ki/Kd` — definida, nunca usada
  - `CureState` struct em `SystemStatus.h` — nunca instanciada (manter apenas se C-07 for implementado)
  - `sysStat.currentPos` e `sysStat.lastPos` — nunca lidos nem escritos
  - `fsCache` para `/mqtt_config.json` e `/temp_config.json` em `FileSystemHandler.cpp` — esses arquivos não existem, cache é dead code

---

## Prioridade 🟡 — Consistência e Qualidade

### C-12 — Idioma misto nas respostas da API
- **Status:** `[ ]`
- **Arquivos:** todos em `src/Endpoints/`
- **Problema:** Mensagens de resposta misturando português e inglês sem critério.
- **Correção:** Padronizar todas as mensagens em pt-BR (idioma do projeto). Fazer busca/substituição em todos os `sendJsonResponse` e `sendErrorResponse`.
- **Exemplos a corrigir:**
  - `"AI configuration fetched successfully"` → `"Configuração de IA obtida com sucesso"`
  - `"System reset successfully"` → `"Sistema resetado com sucesso"`
  - `"Log content fetched successfully"` → `"Conteúdo do log obtido com sucesso"`
  - `"Monitoring data fetched successfully"` → `"Dados de monitoramento obtidos com sucesso"`

---

### C-13 — Senha MQTT exposta no GET `/api/v1/mqtt/config`
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/MQTTConfigEndpoints.cpp:14`
- **Problema:** `doc["mqttPassword"] = systemStatus.mqttPassword` envia a senha em plaintext na resposta HTTP para qualquer cliente na rede.
- **Correção:** Omitir o campo na resposta GET, ou mascarar: `doc["mqttPassword"] = "***"`.

---

### C-14 — Sem validação de range no endpoint de temperatura
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/TemperatureEndpoints.cpp:23`
- **Problema:** PATCH `/api/v1/temperature/config` aceita qualquer valor de `bbqTemperature` sem verificar `minBBQTemp`/`maxBBQTemp` configurados.
- **Correção:**
  ```cpp
  int newBBQTemp = request->getParam("bbqTemperature", true)->value().toInt();
  if (newBBQTemp < systemStatus.minBBQTemp || newBBQTemp > systemStatus.maxBBQTemp) {
      ResponseHelper::sendErrorResponse(request, 400, "Temperatura fora do intervalo permitido");
      return;
  }
  systemStatus.bbqTemperature = newBBQTemp;
  ```
  Aplicar o mesmo para `proteinTemperature`.

---

### C-15 — `TempConfigEndpoints` não valida min < max
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/TempConfigEndpoints.cpp:84`
- **Correção:** Antes de aplicar os novos valores, verificar:
  ```cpp
  if (newMinBBQTemp >= newMaxBBQTemp || newMinPrtTemp >= newMaxPrtTemp ||
      newMinCaliTemp >= newMaxCaliTemp || newMinCaliTempP >= newMaxCaliTempP) {
      ResponseHelper::sendErrorResponse(request, 400, "Valor mínimo deve ser menor que o máximo");
      return;
  }
  ```

---

### C-16 — Campo `"status"` duplicado na resposta `energy/cost` PATCH
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/EnergyEndpoints.cpp:36`
- **Problema:** `jsonDoc["status"] = "success"` dentro do objeto `data` duplica o campo `"status"` que `ResponseHelper` já coloca no nível raiz.
- **Correção:** Remover `jsonDoc["status"] = "success"`. Manter apenas `jsonDoc["kWhCost"] = systemStatus.kWhCost`.

---

### C-17 — Nomes de campos inconsistentes entre endpoints para os mesmos dados
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/MonitorEndpoints.cpp`, `src/Endpoints/TemperatureEndpoints.cpp`
- **Mapeamento atual (inconsistente):**

  | Monitor (`/api/v1/monitor`) | Temperature (`/api/v1/temperature/config`) |
  |---|---|
  | `currentTemp` | `bbqTemperature` (setpoint, não atual!) |
  | `setTemp` | — |
  | `proteinTemp` | `proteinTemperature` (setpoint) |
  | `proteinTempSet` | — |

- **Correção sugerida:** Alinhar nomenclatura e separar claramente `*Current` (leitura do sensor) de `*Setpoint` (temperatura desejada):
  - `bbqCurrentTemp`, `bbqSetpoint`, `proteinCurrentTemp`, `proteinSetpoint`

---

### C-18 — Duas instâncias de `LogHandler` em `main.cpp`
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp:21–22`
- **Problema:**
  ```cpp
  LogHandler logHandler;   // usado em FileSystemHandler, TemperatureControlHandler
  LogHandler _logger;      // usado em todos os outros handlers
  ```
  Dois buffers independentes escrevem no mesmo `/log.txt` sem coordenação. `currentFileSize` de cada instância fica dessincronizado, a rotação pode disparar incorretamente.
- **Correção:** Manter apenas `logHandler` (ou `_logger`), ajustar todos os `extern LogHandler` nos arquivos que referenciam o outro nome para usar uma única instância.

---

## Prioridade 🟡 — Logging

### C-19 — Log apagado em todo boot
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:204`, chamado de `initializeAndLoadConfig()`
- **Problema:** `resetLogFile()` apaga `/log.txt` em cada inicialização. Logs de crashes, OTA failures e resets são perdidos permanentemente.
- **Correção:** Remover a chamada a `resetLogFile()` de `initializeAndLoadConfig()`. Deixar a rotação acontecer apenas quando o arquivo atingir `MAX_LOG_SIZE` (lógica já implementada em `LogHandler::rotateLogFile()`). Opcionalmente, adicionar um marcador de boot no log (`"=== BOOT ==="`).

---

### C-20 — `LogHandler::begin()` nunca chamado
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp`, `src/Handlers/LogHandler.cpp`
- **Problema:** `begin()` inicializa `fileExists` e `currentFileSize` a partir do estado real do arquivo. Sem essa chamada, `currentFileSize = 0` e a rotação por tamanho não funciona corretamente.
- **Correção:** Chamar `logHandler.begin()` no início de `setup()`, após a inicialização do LittleFS.

---

### C-21 — Formatação inconsistente entre `logMessage()` e `writeLog()`
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/LogHandler.cpp`
- **Problema:**
  - `logError("foo")` → chama `logMessage("[ERROR] foo")` → output: `"42s: [ERROR] foo\n"`
  - `logWarning("bar")` → chama `writeLog("WARN", "bar")` → output: `"42s: [WARN] bar\n"`
  - `logMessage("xyz")` → output: `"42s: xyz\n"` (sem nível)
  
  `logMessage()` duplica a lógica de timestamp de `writeLog()` e os dois métodos não compartilham código.
- **Correção:** `logMessage()` passa a chamar `writeLog("INFO", message)`. `logError()` chama `writeLog("ERROR", message)` diretamente (sem passar por `logMessage`). Isso unifica o formato em `"42s: [LEVEL] msg\n"` para todas as chamadas.

---

### C-22 — MQTT `publishAllMessages()` polui o log
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/MQTTHandler.cpp:188–228`
- **Problema:** A cada 3s (intervalo do `mqttTask`), são logadas ~10 linhas incluindo linhas separadoras `"======="`. Com log máximo de 50KB, isso é preenchido em poucas horas de uso.
- **Correção:** Remover os logs individuais de cada publicação e as linhas separadoras de `publishAllMessages()`. Manter apenas um log de erro em caso de falha de publicação. Logs de diagnóstico MQTT já existem em `DiagnosticsHandler`.

---

### C-23 — Senha MQTT logada em plaintext
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:124`
- **Correção:** Remover a linha `logHandler.logMessage("mqttPassword: " + String(status.mqttPassword))`.

---

## Prioridade 🟢 — Performance

### C-24 — `getCurrentPageId()` chamado 4× por loop com delay de 100ms
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/NextionHandler.cpp:238`, `src/main.cpp:62–65`
- **Problema:** `loop()` chama 4 funções de update, cada uma chama `getCurrentPageId()` que contém `delay(100)`. O loop pode bloquear até **400ms** por iteração apenas em leitura serial do Nextion.
- **Correção:** Chamar `getCurrentPageId()` uma única vez em `loop()` e passar o resultado:
  ```cpp
  // main.cpp loop()
  uint32_t currentPage = getCurrentPageId();
  updateNextionMonitorVariables(sysStat, currentPage);
  updateNextionSetBBQVariables(sysStat, currentPage);
  updateNextionSetChunkVariables(sysStat, currentPage);
  updateNextionSetCaliVariables(sysStat, currentPage);
  ```
  Alterar as assinaturas das 4 funções para receber `uint32_t pageId` como parâmetro.

---

### C-25 — `DynamicJsonDocument` em todos os endpoints
- **Status:** `[ ]`
- **Arquivos:** todos em `src/Endpoints/`
- **Problema:** `DynamicJsonDocument(1024)` aloca 1KB no heap em cada requisição. Com múltiplas requisições rápidas (web UI polling) pode causar fragmentação.
- **Correção:** Substituir por `StaticJsonDocument<N>` com tamanho ajustado ao conteúdo real de cada endpoint:
  - Endpoints simples (2–4 campos): `StaticJsonDocument<256>`
  - Monitor (muitos campos): `StaticJsonDocument<512>`
  - `ResponseHelper`: ajustar para aceitar `JsonDocument&` ao invés de `JsonObject`

---

### C-26 — `diagnostics.logMetrics()` e `checkTasks()` chamados em cada tick do loop
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp:70–72`
- **Problema:** Mesmo com throttle interno de 60s, `getMetrics()` chama diversas funções ESP a cada iteração do loop para verificar se é hora de logar.
- **Correção:** Mover `logMetrics()` e `checkTasks()` para uma task FreeRTOS dedicada com intervalo explícito:
  ```cpp
  void diagnosticsTask(void* param) {
      while (true) {
          diagnostics.checkTasks();
          diagnostics.logMetrics();
          vTaskDelay(pdMS_TO_TICKS(60000));
      }
  }
  ```

---

## Segurança

### C-27 — API Key do Google hardcoded no config padrão
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:68`
- **Problema:** `doc["aiKey"] = "AIzaSyDf9K8Ya3djc2PO0YMmJmADRhuYFHMrgbc"` — chave real exposta no código.
- **Correção:** Substituir pelo valor vazio `""` como padrão. O usuário configura via endpoint `/api/v1/ai/config`.

---

### C-28 — Sem autenticação nos endpoints da API
- **Status:** `[ ]`
- **Arquivos:** todos em `src/Endpoints/`
- **Problema:** Qualquer dispositivo na mesma rede pode: resetar o sistema, iniciar OTA, alterar config de temperatura, alterar config MQTT. Não existe nenhum token, basic auth ou verificação de IP.
- **Correção sugerida:** Implementar API key simples via header `X-API-Key`, configurável e persistida. Opcional: restringir endpoints destrutivos (reset, OTA, config) a requisições com a chave correta.

---

## Rastreabilidade

| ID | Arquivo Principal | Prioridade | Status |
|---|---|---|---|
| C-01 | `src/main.cpp`, `src/Handlers/TaskHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-02 | `src/main.cpp`, `src/Handlers/MQTTHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-03 | `include/MQTTHandler.h`, `src/Handlers/MQTTHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-04 | `src/main.cpp`, `src/Handlers/WiFiHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-05 | `src/Handlers/DiagnosticsHandler.cpp` | 🟠 Lógica | `[ ]` |
| C-06 | `src/Handlers/DiagnosticsHandler.cpp` | 🟠 Lógica | `[ ]` |
| C-07 | `src/Endpoints/SystemEndpoints.cpp` | 🟠 Lógica | `[ ]` |
| C-08 | `src/Handlers/OTAHandler.cpp` | 🟠 Lógica | `[ ]` |
| C-09 | `src/Handlers/TemperatureControlHandler.cpp` | 🟠 Lógica | `[ ]` |
| C-10 | `src/Endpoints/MQTTConfigEndpoints.cpp` | 🟠 Lógica | `[ ]` |
| C-11 | `include/TemperatureControl.h`, `include/SystemStatus.h` | 🟠 Lógica | `[ ]` |
| C-12 | `src/Endpoints/*.cpp` | 🟡 Qualidade | `[ ]` |
| C-13 | `src/Endpoints/MQTTConfigEndpoints.cpp` | 🟡 Qualidade | `[ ]` |
| C-14 | `src/Endpoints/TemperatureEndpoints.cpp` | 🟡 Qualidade | `[ ]` |
| C-15 | `src/Endpoints/TempConfigEndpoints.cpp` | 🟡 Qualidade | `[ ]` |
| C-16 | `src/Endpoints/EnergyEndpoints.cpp` | 🟡 Qualidade | `[ ]` |
| C-17 | `src/Endpoints/MonitorEndpoints.cpp` | 🟡 Qualidade | `[ ]` |
| C-18 | `src/main.cpp` | 🟡 Qualidade | `[ ]` |
| C-19 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Logging | `[ ]` |
| C-20 | `src/main.cpp` | 🟡 Logging | `[ ]` |
| C-21 | `src/Handlers/LogHandler.cpp` | 🟡 Logging | `[ ]` |
| C-22 | `src/Handlers/MQTTHandler.cpp` | 🟡 Logging | `[ ]` |
| C-23 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Logging | `[ ]` |
| C-24 | `src/Handlers/NextionHandler.cpp`, `src/main.cpp` | 🟢 Performance | `[ ]` |
| C-25 | `src/Endpoints/*.cpp` | 🟢 Performance | `[ ]` |
| C-26 | `src/main.cpp` | 🟢 Performance | `[ ]` |
| C-27 | `src/Handlers/FileSystemHandler.cpp` | 🔐 Segurança | `[ ]` |
| C-28 | `src/Endpoints/*.cpp` | 🔐 Segurança | `[ ]` |

---

## Ordem de execução sugerida

```
Sprint 1 — Estabilidade (C-01, C-02, C-03, C-04, C-18, C-19, C-20)
Sprint 2 — Lógica e Correção de Bugs (C-05, C-06, C-08, C-09, C-10, C-11)
Sprint 3 — API e Logging (C-12, C-13, C-14, C-15, C-16, C-17, C-21, C-22, C-23)
Sprint 4 — Performance (C-24, C-25, C-26)
Sprint 5 — Segurança e Features pendentes (C-07, C-27, C-28)
```
