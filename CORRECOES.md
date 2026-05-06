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

---

## Segunda Análise — Novos Achados

### C-29 — `client.loop()` nunca chamado no mqttTask — MQTT não recebe mensagens
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/TaskHandler.cpp:66`, `src/Handlers/MQTTHandler.cpp:141`
- **Problema:** `mqttTask` só chama `managePublishing()` → `publishAllMessages()`. O `client.loop()` do PubSubClient é chamado em `MQTTHandler::loop()` e `checkAndReconnectAwsIoT()`, mas nenhum dos dois é invocado da task. Sem `client.loop()`: (a) nenhuma mensagem recebida é processada (subscriptions são inúteis), (b) o keep-alive de 30s nunca é enviado → broker derruba a conexão, (c) a task fica num loop de reconectar/publicar/cair.
- **Correção:** Adicionar `mqttHandler->loop()` (ou `client.loop()` direto) dentro do `mqttTask`, **antes** de `managePublishing()`:
  ```cpp
  void mqttTask(void *parameter) {
      const TickType_t xDelay = pdMS_TO_TICKS(3000);
      while (true) {
          if (systemStatus->isHAAvailable) {
              mqttHandler->loop();          // ← processa keep-alive e mensagens recebidas
              mqttHandler->managePublishing(*systemStatus);
          }
          vTaskDelay(xDelay);
      }
  }
  ```

---

### C-30 — Conflito de page ID 6: `energyPg` vs componentes de calibração
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/NextionHandler.cpp:54`, `src/Handlers/NextionHandler.cpp:80–88`
- **Problema:** A página 6 do Nextion é declarada como `energyPg`, mas os componentes de calibração (`caliBBQTemp`, `caliChunkTemp`, `minCaliBBQTemp`, etc.) também estão mapeados para a página 6. `updateNextionSetCaliVariables()` dispara quando `currentPageId == 6`. Na prática, quando o usuário abre a tela de energia, o firmware tenta atualizar componentes de calibração nela — que não existem nessa página — gerando comandos inválidos enviados ao Nextion.
- **Correção:** Verificar no arquivo `.HMI` do Nextion qual é a página real de calibração e corrigir os IDs no código. Se calibração for página 9 (por exemplo), atualizar a `NexPage` e o check em `updateNextionSetCaliVariables()`.

---

### C-31 — `FileSystem::saveConfig()`, `resetToDefaults()`, `loadConfigFile()`, `saveConfigFile()` declarados mas não implementados
- **Status:** `[ ]`
- **Arquivos:** `include/FileSystem.h`, `src/Handlers/FileSystem.cpp`
- **Problema:** O header declara `saveConfig()`, `resetToDefaults()`, `loadConfigFile()`, `saveConfigFile()`. Apenas `loadConfig()` está em `FileSystem.cpp`, que internamente chama `loadConfigFile()` e `resetToDefaults()` — sem implementação. Se `loadConfig()` for chamada (não é atualmente), o linker falha. É um contrato de interface quebrado.
- **Correção (opção A):** Implementar os métodos faltantes em `FileSystem.cpp` usando o mesmo padrão de `FileSystemHandler.cpp`.
- **Correção (opção B):** Remover `FileSystem.cpp` e os métodos não usados do header, consolidando tudo em `FileSystemHandler.cpp`. Ver também C-32.

---

### C-32 — Dois arquivos `.cpp` implementando a mesma classe `FileSystem`
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/FileSystem.cpp`, `src/Handlers/FileSystemHandler.cpp`
- **Problema:** `FileSystem.cpp` e `FileSystemHandler.cpp` são dois arquivos compilados juntos que implementam métodos da mesma classe `FileSystem`. Além da confusão, os dois usam instâncias diferentes do logger: `FileSystem.cpp` usa `extern LogHandler _logger`, `FileSystemHandler.cpp` usa `extern LogHandler logHandler` — reforçando o bug C-18. `FileSystem.cpp` só contém `loadConfig()`, que não é chamado em lugar nenhum no código atual.
- **Correção:** Remover `FileSystem.cpp`. Todo código útil já está em `FileSystemHandler.cpp`.

---

### C-33 — `kWhCost` não é persistido no config
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:139`
- **Problema:** O endpoint PATCH `/api/v1/energy/cost` atualiza `sysStat.kWhCost`, mas `saveConfigToFile()` não inclui esse campo. Toda alteração do custo do kWh é perdida no reboot.
- **Correção:** Adicionar em `saveConfigToFile()`:
  ```cpp
  doc["kWhCost"] = status.kWhCost;
  ```
  E em `initializeAndLoadConfig()` ao carregar:
  ```cpp
  status.kWhCost = doc["kWhCost"] | 1.0f;  // default 1.0
  ```

---

### C-34 — `client.loop()` não chamado = MQTT clientId aleatório quebra Home Assistant
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/MQTTHandler.cpp:78`
- **Problema:** `String clientId = "BBQ-" + String(random(0xffff), HEX)` gera um ID diferente a cada reconexão. O Home Assistant usa o `clientId` para identificar devices — cada reconexão cria um novo device no HA, acumula entidades duplicadas e quebra automações.
- **Correção:** Usar o `deviceId` (MAC address) já disponível em `sysStat`:
  ```cpp
  String clientId = "BBQ-" + String(systemStatus.deviceId);
  ```

---

### C-35 — `#include <Nextion.h>` desnecessário em 10+ arquivos — acopla logs ao Serial da lib
- **Status:** `[ ]`
- **Arquivos:** `src/Endpoints/AIEndpoints.cpp`, `EnergyEndpoints.cpp`, `MQTTConfigEndpoints.cpp`, `TempConfigEndpoints.cpp`, `src/Handlers/MQTTHandler.cpp`, `OTAHandler.cpp`, `FileSystemHandler.cpp`, `LogHandler.cpp`
- **Problema:** `Nextion.h` → `NexConfig.h` define `#define DEBUG_SERIAL_ENABLE` e `#define dbSerial Serial`. Arquivos que incluem Nextion.h sem usar nada do display herdam essa definição. Pior: `LogHandler.cpp` usa `dbSerial` diretamente — significa que se `DEBUG_SERIAL_ENABLE` for comentada no `NexConfig.h` (algo natural para produção), todos os logs do sistema param silenciosamente sem nenhum erro de compilação.
- **Correção em dois passos:**
  1. Remover `#include <Nextion.h>` de todos os arquivos que não usam componentes Nextion (endpoints, MQTT, OTA, FileSystem).
  2. Em `LogHandler.cpp`, substituir `dbSerial` por `Serial` diretamente — não depender de macro de lib de terceiro.

---

### C-36 — `Serial` (USB CDC) nunca inicializado explicitamente
- **Status:** `[ ]`
- **Arquivos:** `src/main.cpp`
- **Problema:** Nenhum lugar chama `Serial.begin()`. Com `ARDUINO_USB_CDC_ON_BOOT=1` o CDC inicializa automaticamente, mas o `LogHandler` começa a escrever via `dbSerial` dentro de `fastInit()` — antes da inicialização estar garantida. Em alguns contextos de boot rápido, as primeiras mensagens de log podem ser perdidas ou corromper a saída serial.
- **Correção:** Adicionar `Serial.begin(115200)` no início de `fastInit()`, antes de qualquer log.

---

### C-37 — `_checkTimeout()` implementado mas nunca chamado no OTA
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/OTAHandler.cpp:238`, `src/Handlers/OTAHandler.cpp:74`
- **Problema:** `OTAHandler` define `UPDATE_TIMEOUT = 300000` (5 minutos) e implementa `_checkTimeout()`, mas nunca o chama em `writeUpdate()`. Um upload travado pode ficar pendurado indefinidamente, bloqueando o firmware update e deixando o dispositivo num estado intermediário.
- **Correção:** Verificar timeout no início de `writeUpdate()`:
  ```cpp
  bool OTAHandler::writeUpdate(uint8_t* data, size_t len) {
      if (_checkTimeout()) {
          _logger.logError("Timeout do OTA excedido");
          abortUpdate();
          return false;
      }
      // ... resto da lógica
  }
  ```

---

### C-38 — Dependências não usadas em `platformio.ini`
- **Status:** `[ ]`
- **Arquivos:** `platformio.ini`
- **Problema:** Duas bibliotecas estão declaradas em `lib_deps` mas não há nenhum `#include` correspondente no código:
  - `plerup/EspSoftwareSerial` — nenhum `#include <SoftwareSerial.h>` em lugar algum
  - `SD` — nenhum `SD.begin()` ou uso da lib SD
- **Correção:** Remover as duas entradas de `lib_deps`. Reduz tempo de compilação e tamanho do firmware.

---

### C-39 — `-Wno-return-type` suprime bugs reais
- **Status:** `[ ]`
- **Arquivos:** `platformio.ini`
- **Problema:** `build_flags` inclui `-Wno-return-type`, que silencia warnings de funções com tipo de retorno declarado mas sem `return`. Isso pode mascarar funções que retornam lixo de stack. Os outros `-Wno-*` têm justificativa (libs de terceiro), mas `-Wno-return-type` é arriscado para código próprio.
- **Correção:** Remover `-Wno-return-type`. Corrigir os erros que aparecerem (provavelmente poucos). Se necessário para libs externas, usar `-Wno-return-type` apenas para a lib específica via `lib_build_flags`.

---

### C-40 — `deviceId` salvo mas nunca usado no sistema
- **Status:** `[ ]`
- **Arquivos:** `include/SystemStatus.h:49`, `src/Handlers/FileSystemHandler.cpp:72,107`
- **Problema:** O `deviceId` (MAC sem `:`) é gerado uma vez na criação do config, lido de volta e logado — mas nunca é usado em nenhuma lógica do sistema (MQTT, API, OTA). Ocupa espaço na struct e no config sem propósito atual.
- **Correção:** Usar onde faz sentido (ver C-34 para MQTT clientId), ou remover se não houver plano.

---

### C-43 — Remover toda a feature de monitoramento de energia
- **Status:** `[ ]`
- **Decisão:** Feature descontinuada — remover completamente.
- **Tudo que precisa ser deletado:**

  | Arquivo | O que remover |
  |---|---|
  | `src/Endpoints/EnergyEndpoints.cpp` | deletar arquivo inteiro |
  | `include/EnergyEndpoints.h` | deletar arquivo inteiro |
  | `src/Handlers/WebServerHandler.cpp` | remover `#include "EnergyEndpoints.h"` e `registerEnergyEndpoints(...)` |
  | `include/SystemStatus.h:66–69` | remover campos `power`, `energy`, `cost`, `kWhCost` |
  | `src/Handlers/TemperatureControlHandler.cpp:214–216` | remover zeragem de `power`, `energy`, `cost` em `resetSystem()` |
  | `src/Handlers/NextionHandler.cpp:54` | remover `NexPage energyPg = NexPage(6, 0, "energyPg")` |
  | `src/Handlers/MonitorEndpoints.cpp` | verificar se expõe campos de energia e remover |

- **Obs:** A remoção de `energyPg` (page 6) resolve parcialmente o C-30 (conflito de page ID). Após remover, confirmar qual é a página real de calibração e corrigir o mapeamento.

---

### C-41 — Diretórios `src/Webhooks/` e `include/Webhooks/` vazios
- **Status:** `[ ]`
- **Arquivos:** `src/Webhooks/`, `include/Webhooks/`
- **Problema:** Diretórios criados para uma feature que nunca foi implementada. Poluem a estrutura do projeto.
- **Correção:** Remover ambos os diretórios.

---

### C-42 — README descreve hardware errado (ACS712 vs MAX6675)
- **Status:** `[ ]`
- **Arquivos:** `README.md`
- **Problema:** O README menciona `"Sensores de Temperatura ACS712"`, mas ACS712 é um **sensor de corrente**, não de temperatura. O código usa `MAX6675` (termopar) para temperatura de câmara e proteína, e `DS18B20` para temperatura interna.
- **Correção:** Atualizar o README com o hardware real: MAX6675 (x2) para termopares + DS18B20 para temperatura interna do ESP.

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
| C-29 | `src/Handlers/TaskHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-30 | `src/Handlers/NextionHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-31 | `include/FileSystem.h`, `src/Handlers/FileSystem.cpp` | 🟠 Lógica | `[ ]` |
| C-32 | `src/Handlers/FileSystem.cpp` | 🟠 Lógica | `[ ]` |
| C-33 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Qualidade | `[ ]` |
| C-34 | `src/Handlers/MQTTHandler.cpp` | 🟡 Qualidade | `[ ]` |
| C-35 | `src/Endpoints/*.cpp`, `src/Handlers/LogHandler.cpp` | 🟡 Qualidade | `[ ]` |
| C-36 | `src/main.cpp` | 🟡 Qualidade | `[ ]` |
| C-37 | `src/Handlers/OTAHandler.cpp` | 🟠 Lógica | `[ ]` |
| C-38 | `platformio.ini` | 🟢 Performance | `[ ]` |
| C-39 | `platformio.ini` | 🟠 Lógica | `[ ]` |
| C-40 | `include/SystemStatus.h` | 🟡 Qualidade | `[ ]` |
| C-41 | `src/Webhooks/`, `include/Webhooks/` | 🟡 Qualidade | `[ ]` |
| C-42 | `README.md` | 🟡 Qualidade | `[ ]` |
| C-43 | `src/Endpoints/EnergyEndpoints.cpp`, `include/SystemStatus.h`, `src/Handlers/NextionHandler.cpp` | 🗑️ Remoção | `[ ]` |

---

## Ordem de execução sugerida

```
Sprint 1 — Estabilidade crítica
  C-01  tasks duplicadas (race condition)
  C-02  MQTTHandler::begin() nunca chamado
  C-03  processMessage() não implementado
  C-04  initWiFi() nunca chamado
  C-18  duas instâncias de LogHandler
  C-29  client.loop() ausente no mqttTask
  C-30  conflito de page ID no Nextion (energia vs calibração)
  C-32  FileSystem.cpp duplicado — remover
  C-36  Serial.begin() ausente

Sprint 2 — Lógica e bugs
  C-05  handle de task deletada no vetor
  C-06  isHealthy() bytes vs %
  C-08  OTA verifica partição errada
  C-09  histerese assimétrica
  C-10  MQTT task não inicia/para
  C-11  código morto (updateRelayState, pidControl, etc.)
  C-31  métodos não implementados em FileSystem
  C-37  timeout OTA nunca verificado
  C-39  -Wno-return-type mascara bugs

Sprint 3 — API, persistência e logging
  C-12  idioma misto nas respostas
  C-13  senha MQTT exposta no GET
  C-14  sem validação de range na temperatura
  C-15  TempConfig não valida min < max
  C-16  campo status duplicado no energy
  C-17  nomes inconsistentes entre endpoints
  C-19  log apagado em todo boot
  C-20  LogHandler::begin() nunca chamado
  C-21  logMessage vs writeLog formatação
  C-22  MQTT polui log
  C-23  senha MQTT no log
  C-33  kWhCost não persistido
  C-34  clientId MQTT aleatório quebra HA

Sprint 4 — Qualidade e limpeza
  C-24  getCurrentPageId() 4x por loop
  C-25  DynamicJsonDocument em todos endpoints
  C-26  diagnostics no loop Arduino
  C-35  Nextion.h desnecessário em 10+ arquivos
  C-38  libs não usadas (SoftwareSerial, SD)
  C-40  deviceId salvo mas nunca usado
  C-41  diretórios Webhooks vazios
  C-42  README com hardware errado
  C-43  remover feature de energia (power/energy/cost/kWhCost) — descontinuada

Sprint 5 — Segurança e features pendentes
  C-07  cureProcessMode sem implementação
  C-27  API key hardcoded
  C-28  sem autenticação na API
```
