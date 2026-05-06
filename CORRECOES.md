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
- **Status:** `[x]`
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
- **Status:** `[x]`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp:138`
- **Problema:** `metrics.freeStack = uxTaskGetStackHighWaterMark(NULL)` retorna bytes disponíveis no stack da task atual (high water mark). O comentário diz "20%" mas o threshold `> 20` é irrisório (20 bytes).
- **Correção:** Definir threshold em bytes condizente com os stacks configurados (`TEMP_TASK_STACK = 3072`, `CONTROL_TASK_STACK = 2048`). Exemplo: `bool stackOk = metrics.freeStack > 256;` — ou calcular a percentagem a partir do tamanho total do stack, o que requer passar o tamanho total como parâmetro. Corrigir também o comentário.

---

### C-07 — `cureProcessMode` ativado mas nunca verificado
- **Status:** `[x]`
- **Arquivos:** `src/Endpoints/SystemEndpoints.cpp:13`, `src/Handlers/TemperatureControlHandler.cpp`
- **Problema:** `POST /api/v1/system/activateCure` seta `cureProcessMode = true`, mas nenhuma parte do `controlTemperature()` ou de qualquer task verifica esse flag. A feature é um stub sem efeito.
- **Correção (opção A):** Implementar a lógica de cura (ex: perfil de temperatura por estágios usando `CureState`).
- **Correção (opção B):** Remover o endpoint e a flag enquanto não implementado, para não gerar confusão.

---

### C-08 — `OTAHandler::verifyFirmware()` verifica a partição errada
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
- **Arquivos:** `src/Endpoints/MQTTConfigEndpoints.cpp:14`
- **Problema:** `doc["mqttPassword"] = systemStatus.mqttPassword` envia a senha em plaintext na resposta HTTP para qualquer cliente na rede.
- **Correção:** Omitir o campo na resposta GET, ou mascarar: `doc["mqttPassword"] = "***"`.

---

### C-14 — Sem validação de range no endpoint de temperatura
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
- **Arquivos:** `src/Endpoints/EnergyEndpoints.cpp:36`
- **Problema:** `jsonDoc["status"] = "success"` dentro do objeto `data` duplica o campo `"status"` que `ResponseHelper` já coloca no nível raiz.
- **Correção:** Remover `jsonDoc["status"] = "success"`. Manter apenas `jsonDoc["kWhCost"] = systemStatus.kWhCost`.

---

### C-17 — Nomes de campos inconsistentes entre endpoints para os mesmos dados
- **Status:** `[x]`
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
- **Status:** `[x]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:204`, chamado de `initializeAndLoadConfig()`
- **Problema:** `resetLogFile()` apaga `/log.txt` em cada inicialização. Logs de crashes, OTA failures e resets são perdidos permanentemente.
- **Correção:** Remover a chamada a `resetLogFile()` de `initializeAndLoadConfig()`. Deixar a rotação acontecer apenas quando o arquivo atingir `MAX_LOG_SIZE` (lógica já implementada em `LogHandler::rotateLogFile()`). Opcionalmente, adicionar um marcador de boot no log (`"=== BOOT ==="`).

---

### C-20 — `LogHandler::begin()` nunca chamado
- **Status:** `[x]`
- **Arquivos:** `src/main.cpp`, `src/Handlers/LogHandler.cpp`
- **Problema:** `begin()` inicializa `fileExists` e `currentFileSize` a partir do estado real do arquivo. Sem essa chamada, `currentFileSize = 0` e a rotação por tamanho não funciona corretamente.
- **Correção:** Chamar `logHandler.begin()` no início de `setup()`, após a inicialização do LittleFS.

---

### C-21 — Formatação inconsistente entre `logMessage()` e `writeLog()`
- **Status:** `[x]`
- **Arquivos:** `src/Handlers/LogHandler.cpp`
- **Problema:**
  - `logError("foo")` → chama `logMessage("[ERROR] foo")` → output: `"42s: [ERROR] foo\n"`
  - `logWarning("bar")` → chama `writeLog("WARN", "bar")` → output: `"42s: [WARN] bar\n"`
  - `logMessage("xyz")` → output: `"42s: xyz\n"` (sem nível)
  
  `logMessage()` duplica a lógica de timestamp de `writeLog()` e os dois métodos não compartilham código.
- **Correção:** `logMessage()` passa a chamar `writeLog("INFO", message)`. `logError()` chama `writeLog("ERROR", message)` diretamente (sem passar por `logMessage`). Isso unifica o formato em `"42s: [LEVEL] msg\n"` para todas as chamadas.

---

### C-22 — MQTT `publishAllMessages()` polui o log
- **Status:** `[x]`
- **Arquivos:** `src/Handlers/MQTTHandler.cpp:188–228`
- **Problema:** A cada 3s (intervalo do `mqttTask`), são logadas ~10 linhas incluindo linhas separadoras `"======="`. Com log máximo de 50KB, isso é preenchido em poucas horas de uso.
- **Correção:** Remover os logs individuais de cada publicação e as linhas separadoras de `publishAllMessages()`. Manter apenas um log de erro em caso de falha de publicação. Logs de diagnóstico MQTT já existem em `DiagnosticsHandler`.

---

### C-23 — Senha MQTT logada em plaintext
- **Status:** `[x]`
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
- **Status:** `[x]`
- **Arquivos:** `src/Handlers/FileSystemHandler.cpp:68`
- **Problema:** `doc["aiKey"] = "AIzaSyDf9K8Ya3djc2PO0YMmJmADRhuYFHMrgbc"` — chave real exposta no código.
- **Correção:** Substituir pelo valor vazio `""` como padrão. O usuário configura via endpoint `/api/v1/ai/config`.

---

### C-28 — Sem autenticação nos endpoints da API
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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
- **Status:** `[x]`
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

---

## Terceira Análise — Estrutura do Projeto

### E-01 — `include/` flat vs `src/` com subpastas — assimetria estrutural
- **Status:** `[ ]`
- **Problema:** `src/` está bem dividido em `Endpoints/` e `Handlers/`, mas `include/` tem todos os 20 headers na raiz, sem subpastas. Paradoxalmente, `include/Endpoints/`, `include/Handlers/` e `include/Webhooks/` **existem mas estão completamente vazias** — foram criadas com intenção de espelhar `src/` mas nunca usadas.
- **Impacto:** Para encontrar o header de `src/Endpoints/AIEndpoints.cpp` é preciso saber que ele está em `include/AIEndpoints.h` (raiz), não em `include/Endpoints/`. Isso quebra a descoberta por navegação.
- **Correção:** Mover headers para as subpastas correspondentes e atualizar os includes:
  ```
  include/Endpoints/  ← AIEndpoints.h, MonitorEndpoints.h, etc.
  include/Handlers/   ← MQTTHandler.h, LogHandler.h, etc.
  include/            ← só PinDefinitions.h, SystemStatus.h (globais)
  ```

---

### E-02 — `ResponseHelper.h` dentro de `src/Endpoints/`
- **Status:** `[ ]`
- **Arquivo:** `src/Endpoints/ResponseHelper.h`
- **Problema:** Único header dentro de `src/` — vai contra a convenção do projeto de manter headers em `include/`. Funciona apenas porque os `.cpp` da mesma pasta o encontram pelo caminho relativo. Se um Handler precisar usar ResponseHelper no futuro, não o encontraria.
- **Correção:** Mover para `include/Endpoints/ResponseHelper.h` (junto com E-01).

---

### E-03 — Nomes de arquivo desalinhados entre `.h` e `.cpp`
- **Status:** `[ ]`
- **Problema:** Três pares com nomes que não batem:

  | Header | Implementação | Inconsistência |
  |---|---|---|
  | `TemperatureControl.h` | `TemperatureControlHandler.cpp` | sufixo `Handler` só no .cpp |
  | `WebServerControl.h` | `WebServerHandler.cpp` | `Control` no .h, `Handler` no .cpp |
  | `FileSystem.h` | `FileSystem.cpp` + `FileSystemHandler.cpp` | dois .cpp para um .h |

- **Correção:** Padronizar para `NomeHandler.h` / `NomeHandler.cpp` em tudo:
  - `TemperatureControl.h` → `TemperatureHandler.h` (e renomear .cpp)
  - `WebServerControl.h` → `WebServerHandler.h`
  - Consolidar `FileSystem.cpp` em `FileSystemHandler.cpp` (ver C-32)

---

### E-04 — Include guards inconsistentes
- **Status:** `[ ]`
- **Problema:** A maioria usa `SCREAMING_SNAKE_CASE`, mas dois headers usam `camelCase_h`:

  | Header | Guard atual | Padrão esperado |
  |---|---|---|
  | `MonitorEndpoints.h` | `MonitorEndpoints_h` | `MONITOR_ENDPOINTS_H` |
  | `WebServerControl.h` | `WebServerControl_h` | `WEB_SERVER_CONTROL_H` |

- **Correção:** Atualizar os dois headers para o padrão `SCREAMING_SNAKE_CASE`.

---

### E-05 — Inclusão inconsistente de `AsyncTCP.h` nos headers
- **Status:** `[ ]`
- **Problema:** 7 headers incluem `AsyncTCP.h` antes de `ESPAsyncWebServer.h`, mas 4 usam `ESPAsyncWebServer.h` sem incluir `AsyncTCP.h` (`AIEndpoints.h`, `DiagnosticsEndpoints.h`, `SystemEndpoints.h`, `LogHandler.h`). `AsyncTCP.h` é uma dependência interna do `ESPAsyncWebServer-esphome` e não precisa ser incluída diretamente — os headers que a incluem são desnecessariamente verbosos.
- **Correção:** Remover `#include <AsyncTCP.h>` de todos os headers do projeto. `ESPAsyncWebServer.h` já resolve essa dependência internamente.

---

### E-06 — `extern` espalhados — acoplamento oculto entre arquivos
- **Status:** `[ ]`
- **Problema:** 10 declarações `extern` espalhadas pelos `.cpp`, criando dependências implícitas que não aparecem nas assinaturas das funções. Agravado pelo uso de dois nomes diferentes para o mesmo objeto:

  | Arquivo | Extern usado |
  |---|---|
  | `FileSystem.cpp`, `TemperatureControlHandler.cpp`, `FileSystemHandler.cpp` | `extern LogHandler logHandler` |
  | `MQTTHandler.cpp`, `NextionHandler.cpp`, `OTAHandler.cpp`, `TaskHandler.cpp`, `WiFiHandler.cpp`, `FileSystem.cpp` | `extern LogHandler _logger` |

  `TaskHandler.cpp` vai além: usa `extern SystemStatus sysStat` (global direto) E `static SystemStatus* systemStatus` (ponteiro recebido por parâmetro) — dois padrões para o mesmo objeto dentro do mesmo arquivo.

- **Correção:** Eliminar todos os `extern` de objetos globais. Passar as dependências como parâmetros de função ou via construtor. O único `extern` legítimo neste projeto é para variáveis de hardware (como os objetos Nextion declarados no `.h` e definidos no `.cpp`).

---

### E-07 — `SystemStatus` é uma god struct com 40+ campos sem agrupamento
- **Status:** `[ ]`
- **Arquivo:** `include/SystemStatus.h`
- **Problema:** Uma única struct flat mistura dados de domínios completamente diferentes sem nenhuma organização interna:
  - Temperatura BBQ (leitura, setpoint, amostras, calibração, média)
  - Temperatura proteína (idem)
  - Temperatura interna do ESP
  - Controle do relay
  - Config MQTT (servidor, porta, usuário, senha, flag HA)
  - Config AI (chave, tip)
  - Config de limites (min/max BBQ, proteína, calibração — 8 campos)
  - Energia (power, cost — a remover via C-43)
  - Estado de processo (cureProcessMode, currentPos, lastPos — mortos)

- **Correção sugerida:** Agrupar em sub-structs semânticas dentro de `SystemStatus` (sem quebrar a API existente):
  ```cpp
  struct SystemStatus {
      struct { int calibrated; int setpoint; int calibration; ... } bbq;
      struct { int calibrated; int setpoint; int calibration; ... } protein;
      struct { char server[100]; int port; char user[30]; ... } mqtt;
      struct { char key[128]; char tip[256]; } ai;
      struct { int minBBQ; int maxBBQ; int minPrt; ... } limits;
      bool isRelayOn;
      // ...
  };
  ```

---

### E-08 — `NextionHandler.cpp` com 437 linhas acumula 4 responsabilidades
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/NextionHandler.cpp`
- **Problema:** O maior arquivo do projeto mistura:
  1. **Mapeamento de hardware** — definição de todos os objetos `NexPage`, `NexNumber`, `NexButton` (linhas 48–97)
  2. **Callbacks de eventos** — `setBBQTempPushCallback`, `setChunkTempPushCallback`, `setStopPushCallback`, `setCaliPushCallback` (linhas 99–218)
  3. **Lógica de update por página** — `updateNextionMonitorVariables`, `updateNextionSetBBQVariables`, etc. (linhas 292–437)
  4. **Helpers de comunicação serial** — `getCurrentPageId`, `setPageBackground`, `updateNumberComponent` (linhas 238–290)

  Além disso, 4 structs/variáveis de cache separadas no topo do arquivo (linhas 10–45) — uma por página — em vez de um cache unificado.

- **Correção:** Dividir em pelo menos dois arquivos:
  - `NextionComponents.cpp/.h` — declaração de todos os objetos Nextion (o mapeamento de hardware)
  - `NextionHandler.cpp/.h` — lógica de init, callbacks, update e helpers

---

### E-09 — Sufixos `Handler` / `Control` / sem sufixo sem critério
- **Status:** `[ ]`
- **Problema:** A nomenclatura dos módulos não segue uma convenção clara:

  | Classe/módulo | Sufixo | Tipo real |
  |---|---|---|
  | `LogHandler`, `MQTTHandler`, `OTAHandler`, `DiagnosticsHandler`, `NextionHandler`, `WiFiHandler`, `TaskHandler` | `Handler` | Varia: alguns são services, outros são drivers |
  | `WebServerControl` | `Control` | É um registrador de endpoints |
  | `TemperatureControl` (header) | `Control` | É um conjunto de funções puras |
  | `FileSystem` | nenhum | É um service de persistência |

- **Correção:** Adotar uma convenção e aplicar consistentemente. Sugestão para este projeto embarcado:
  - `*Handler` — módulo que trata eventos/comunicação com periférico (`NextionHandler`, `MQTTHandler`, `WiFiHandler`)
  - `*Controller` — módulo com lógica de negócio ativa (`TemperatureController`, `WebServerController`)
  - `*Service` ou sem sufixo — módulo de utilidade (`FileSystem`, `LogHandler` → `Logger`)

---

### E-10 — Configurações de sistema espalhadas em múltiplos locais
- **Status:** `[ ]`
- **Problema:** Constantes de configuração estão definidas em 6 arquivos diferentes sem um lugar central:

  | Constante | Onde está |
  |---|---|
  | `NUM_SAMPLES=20`, `MOVING_AVERAGE_SIZE=180` | `SystemStatus.h` |
  | `TEMP_READ_INTERVAL=500`, `PID_UPDATE_INTERVAL=100` | `TemperatureControl.h` |
  | `TEMP_TASK_STACK=3072`, `CONTROL_TASK_STACK=2048` | `TaskHandler.cpp` |
  | `WDT_TIMEOUT_SECONDS=30`, `SOFT_WDT_INTERVAL=60000` | `DiagnosticsHandler.h` |
  | `FIRMWARE_VERSION="1.0.0"`, `MAX_FIRMWARE_SIZE` | `OTAHandler.h` |
  | `MQTT_BUFFER_SIZE=1024`, `MQTT_RETRY_INTERVAL=5000` | `MQTTHandler.cpp` |
  | `LOG_BUFFER_SIZE=1024`, `MAX_LOG_SIZE=50000` | `LogHandler.h` |

- **Correção:** Criar `include/Config.h` com todas as constantes tunáveis do sistema. Deixar em cada header apenas as constantes de implementação interna.

---

---

## Dependências — Atualizações Disponíveis

### D-01 — espressif32 platform 6.10.0 → 7.0.0 (Major)
- **Status:** `[ ]`
- **Risco:** 🔴 Alto — major update do Arduino core ESP32. Pode mudar APIs internas, comportamento de periféricos, pinout de funções, partição padrão.
- **Oportunidade:** Com 7.0.0, testar se `esp32-s3-devkitc-1-n16r8` passa a ser incluída nativamente (eliminando a necessidade de `boards/`). O `pio boards` já listou o board nessa versão.
- **Como atualizar:** Em `platformio.ini`, trocar `platform = espressif32` por `platform = espressif32@7.0.0`.
- **Procedimento:** Atualizar em branch isolado → compilar → gravar no hardware → validar: WiFi conecta, Nextion responde, temperaturas lidas, relay funciona, OTA ativo.

---

### D-02 — ESPAsyncWebServer-esphome 3.3.0 → 3.4.1 (Minor)
- **Status:** `[ ]`
- **Risco:** 🟢 Baixo — minor update, backward-compatible.
- **Como atualizar:** Em `platformio.ini`, trocar `^3.2.2` por `^3.4.1`.

---

### D-03 — ArduinoJson 7.3.0 → 7.4.3 (Minor/Patch)
- **Status:** `[ ]`
- **Risco:** 🟢 Baixo — mesmo major version (7.x), backward-compatible. Possíveis melhorias de performance e correções.
- **Como atualizar:** Em `platformio.ini`, `^7.2.0` → `^7.4.3`.

---

### D-04 — DallasTemperature 3.11.0 → 4.0.6 (Major)
- **Status:** `[ ]`
- **Risco:** 🟡 Médio — major update. A API pública pode ter mudado. Verificar se `sensors.getTempCByIndex(0)` e `sensors.requestTemperatures()` continuam iguais.
- **Uso no projeto:** `src/Handlers/TemperatureControlHandler.cpp` → `getCalibratedInternalTemp()` (DS18B20 interno).
- **Como atualizar:** Em `platformio.ini`, trocar `^3.11.0` por `^4.0.6`. Compilar e verificar warnings/errors.

---

### Dependências em dia (nenhuma ação necessária)
| Biblioteca | Versão atual | Status |
|---|---|---|
| Nextion (itead) | 0.9.0 | ✅ última versão |
| PubSubClient | 2.8.0 | ✅ última versão |
| WiFiManager | 2.0.17 | ✅ última versão |
| MAX6675 library | 1.1.2 | ✅ última versão |
| OneWire | 2.3.8 | ✅ última versão |
| EspSoftwareSerial | 8.2.0 | ✅ (a remover — C-38) |

---

---

## Análise Kimi-K2.6 — Concorrência, Hardware e Runtime

*Análise gerada em 2026-05-06 com base no estado pós-Sprint 1. K-10 já resolvido na Sprint 4 (C-35).*

---

### K-01 — `SystemStatus` sem mecanismo de sincronização
- **Status:** `[ ]`
- **Arquivos:** `include/SystemStatus.h`, todos os `.cpp` que acessam `sysStat`
- **Problema:** `sysStat` é acessada por ~5 tasks + callbacks Nextion + handlers de endpoint sem nenhum mutex, `volatile` ou `std::atomic`. O compilador pode reordenar acessos e o cache de cada core pode ver valores desatualizados. Campos `float` e structs maiores podem sofrer *tearing*.
- **Impacto:** Temperatura alvo, estado do relé e amostras lidas de forma inconsistente entre cores. Bugs intermitentes impossíveis de reproduzir em debug.
- **Correção:** Criar `SemaphoreHandle_t sysStatMutex` global. Envolver toda leitura/escrita em `sysStat` com `xSemaphoreTake` / `xSemaphoreGive`. Para `isRelayOn`, considerar `std::atomic<bool>`.

---

### K-02 — Hardware WDT não monitora `TempTask` nem `ControlTask`
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/DiagnosticsHandler.cpp:10-13`, `src/Handlers/TaskHandler.cpp`
- **Problema:** `esp_task_wdt_add(NULL)` no construtor de `DiagnosticsHandler` registra apenas a task corrente no momento (tipicamente `loop()`, core 1). `TempTask` e `ControlTask` (core 0) nunca são registradas no WDT.
- **Impacto:** Se `temperatureTask` ou `controlTask` travarem (deadlock em SPI, sensor preso), o hardware watchdog não detecta. O relé pode ficar preso em ON indefinidamente.
- **Correção:** Em `TaskHandler.cpp`, após `xTaskCreatePinnedToCore`, chamar `esp_task_wdt_add(tempTaskHandle)` e `esp_task_wdt_add(controlTaskHandle)`. Cada task deve chamar `esp_task_wdt_reset()` a cada iteração.

---

### K-03 — `LogHandler` buffer sem mutex
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/LogHandler.cpp`
- **Problema:** `logHandler` é chamado concorrentemente por `TempTask`, `ControlTask`, `MQTTTask` e `loop()`. O buffer `logBuffer[1024]`, `bufferIndex`, `currentFileSize` são acessados sem mutex. Duas tasks podem executar `memcpy(logBuffer + bufferIndex, ...)` simultaneamente.
- **Impacto:** Corrupção do buffer, índice inválido (`bufferIndex > LOG_BUFFER_SIZE`), crash ou log corrompido no LittleFS.
- **Correção:** Adicionar `SemaphoreHandle_t _logMutex` em `LogHandler` e envolver `writeLog`, `flushBuffer` e `clearLogs` com `xSemaphoreTake` / `xSemaphoreGive`.

---

### K-04 — `Update.end(true)` aceita firmware incompleto
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/OTAHandler.cpp:95-114`
- **Problema:** `Update.end(true)` — o parâmetro `true` (*evenIfRemaining*) faz o ESP32 aceitar o firmware mesmo que nem todos os bytes tenham sido escritos.
- **Impacto:** Upload truncado gera boot com firmware corrompido. Possível brick ou loop de boot.
- **Correção:** Trocar `Update.end(true)` por `Update.end(false)`. Uma linha.

---

### K-05 — DS18B20 sem validação de valores de erro
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp:36-43`
- **Problema:** `sensors.getTempCByIndex(0)` retorna `-127.0` (sensor desconectado) ou `85.0` (power-on reset). O código converte diretamente para `int`, propagando o erro como temperatura real.
- **Impacto:** Temperatura interna exibida como -127°C ou 85°C; se usada em lógica de corte, comportamento errático.
- **Correção:**
  ```cpp
  float temp = sensors.getTempCByIndex(0);
  if (temp == DEVICE_DISCONNECTED_C || temp == 85.0f) return sysStat.calibratedTempInternal;
  sysStat.calibratedTempInternal = (int)round(temp);
  ```

---

### K-06 — MAX6675 sem validação de leitura inválida
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/TemperatureControlHandler.cpp:46-74`
- **Problema:** `MAX6675::readCelsius()` pode retornar `NaN` ou `0` em falha SPI ou termopar aberto. O código soma calibração e insere na média móvel sem validar.
- **Impacto:** Média móvel corrompida com `NaN` ou zeros. Relé pode ligar/desligar incorretamente.
- **Correção:**
  ```cpp
  float raw = thermocouple.readCelsius();
  if (isnan(raw) || raw <= 0 || raw > 500) return sysStat.calibratedTemp;
  float temp = raw + sysStat.tempCalibration;
  ```

---

### K-07 — `neopixelWrite` chamado de múltiplas tasks sem sincronização
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/TemperatureControlHandler.cpp`, `src/main.cpp`, `src/Handlers/WiFiHandler.cpp`
- **Problema:** `neopixelWrite()` usa o periférico RMT do ESP32-S3. Chamada concorrente de `loop()`, `TempTask`, `ControlTask` e callbacks WiFi pode causar glitch visual ou corrupção do driver RMT.
- **Impacto:** LED RGB errático; em casos extremos, crash do driver RMT.
- **Correção:** Centralizar o controle do LED em uma única função chamada apenas do `loop()` com base no estado atual de `sysStat.isRelayOn`, removendo as chamadas de dentro das tasks.

---

### K-08 — `String` em hot paths causando fragmentação de heap
- **Status:** `[ ]`
- **Arquivos:** `src/Handlers/LogHandler.cpp`, `src/Handlers/MQTTHandler.cpp`
- **Problema:** Concatenações de `String` em `writeLog()` (`timeStamp + "[" + level + "]"...`) alocam no heap a cada ciclo. Em operação prolongada, o heap fragmenta e alocações passam a falhar.
- **Impacto:** Falha silenciosa de alocação → log não grava ou crash por `panic`.
- **Correção:** Substituir por `snprintf` em buffer `char[]` fixo no stack.

---

### K-09 — Divisão por zero no cálculo de fragmentação de heap
- **Status:** `[ ]`
- **Arquivo:** `src/Handlers/DiagnosticsHandler.cpp:122`
- **Problema:** `heapFragmentation = 100 - (maxAllocHeap * 100) / freeHeap` — se `freeHeap == 0`, divisão por zero. Em float não crasha, mas gera `NaN`/`Inf`.
- **Correção:** Uma linha:
  ```cpp
  metrics.heapFragmentation = (metrics.freeHeap == 0) ? 100.0f
      : 100.0f - ((float)metrics.maxAllocHeap * 100.0f) / metrics.freeHeap;
  ```

---

### K-10 — `LogHandler` dependia de macro `dbSerial` de lib externa
- **Status:** `[x]`
- **Resolvido em:** Sprint 4 (C-35) — `dbSerial` substituído por `Serial` diretamente.

---

## Rastreabilidade

| ID | Arquivo Principal | Prioridade | Status |
|---|---|---|---|
| C-01 | `src/main.cpp`, `src/Handlers/TaskHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-02 | `src/main.cpp`, `src/Handlers/MQTTHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-03 | `include/MQTTHandler.h`, `src/Handlers/MQTTHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-04 | `src/main.cpp`, `src/Handlers/WiFiHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-05 | `src/Handlers/DiagnosticsHandler.cpp` | 🟠 Lógica | `[x]` |
| C-06 | `src/Handlers/DiagnosticsHandler.cpp` | 🟠 Lógica | `[x]` |
| C-07 | `src/Endpoints/SystemEndpoints.cpp` | 🟠 Lógica | `[x]` |
| C-08 | `src/Handlers/OTAHandler.cpp` | 🟠 Lógica | `[x]` |
| C-09 | `src/Handlers/TemperatureControlHandler.cpp` | 🟠 Lógica | `[x]` |
| C-10 | `src/Endpoints/MQTTConfigEndpoints.cpp` | 🟠 Lógica | `[x]` |
| C-11 | `include/TemperatureControl.h`, `include/SystemStatus.h` | 🟠 Lógica | `[x]` |
| C-12 | `src/Endpoints/*.cpp` | 🟡 Qualidade | `[x]` |
| C-13 | `src/Endpoints/MQTTConfigEndpoints.cpp` | 🟡 Qualidade | `[x]` |
| C-14 | `src/Endpoints/TemperatureEndpoints.cpp` | 🟡 Qualidade | `[x]` |
| C-15 | `src/Endpoints/TempConfigEndpoints.cpp` | 🟡 Qualidade | `[x]` |
| C-16 | `src/Endpoints/EnergyEndpoints.cpp` | 🟡 Qualidade | `[x]` |
| C-17 | `src/Endpoints/MonitorEndpoints.cpp` | 🟡 Qualidade | `[x]` |
| C-18 | `src/main.cpp` | 🟡 Qualidade | `[x]` |
| C-19 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Logging | `[x]` |
| C-20 | `src/main.cpp` | 🟡 Logging | `[x]` |
| C-21 | `src/Handlers/LogHandler.cpp` | 🟡 Logging | `[x]` |
| C-22 | `src/Handlers/MQTTHandler.cpp` | 🟡 Logging | `[x]` |
| C-23 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Logging | `[x]` |
| C-24 | `src/Handlers/NextionHandler.cpp`, `src/main.cpp` | 🟢 Performance | `[x]` |
| C-25 | `src/Endpoints/*.cpp` | 🟢 Performance | `[x]` |
| C-26 | `src/main.cpp` | 🟢 Performance | `[x]` |
| C-27 | `src/Handlers/FileSystemHandler.cpp` | 🔐 Segurança | `[x]` |
| C-28 | `src/Endpoints/*.cpp` | 🔐 Segurança | `[x]` |
| C-29 | `src/Handlers/TaskHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-30 | `src/Handlers/NextionHandler.cpp` | 🔴 Crítico | `[ ]` |
| C-31 | `include/FileSystem.h`, `src/Handlers/FileSystem.cpp` | 🟠 Lógica | `[x]` |
| C-32 | `src/Handlers/FileSystem.cpp` | 🟠 Lógica | `[x]` |
| C-33 | `src/Handlers/FileSystemHandler.cpp` | 🟡 Qualidade | `[x]` |
| C-34 | `src/Handlers/MQTTHandler.cpp` | 🟡 Qualidade | `[x]` |
| C-35 | `src/Endpoints/*.cpp`, `src/Handlers/LogHandler.cpp` | 🟡 Qualidade | `[x]` |
| C-36 | `src/main.cpp` | 🟡 Qualidade | `[x]` |
| C-37 | `src/Handlers/OTAHandler.cpp` | 🟠 Lógica | `[x]` |
| C-38 | `platformio.ini` | 🟢 Performance | `[x]` |
| C-39 | `platformio.ini` | 🟠 Lógica | `[x]` |
| C-40 | `include/SystemStatus.h` | 🟡 Qualidade | `[x]` |
| C-41 | `src/Webhooks/`, `include/Webhooks/` | 🟡 Qualidade | `[x]` |
| C-42 | `README.md` | 🟡 Qualidade | `[x]` |
| C-43 | `src/Endpoints/EnergyEndpoints.cpp`, `include/SystemStatus.h`, `src/Handlers/NextionHandler.cpp` | 🗑️ Remoção | `[x]` |
| E-01 | `include/`, `src/Endpoints/`, `src/Handlers/` | 🏗️ Estrutura | `[ ]` |
| E-02 | `src/Endpoints/ResponseHelper.h` | 🏗️ Estrutura | `[ ]` |
| E-03 | `TemperatureControl.h`, `WebServerControl.h`, `FileSystem.h` | 🏗️ Estrutura | `[ ]` |
| E-04 | `include/MonitorEndpoints.h`, `include/WebServerControl.h` | 🏗️ Estrutura | `[ ]` |
| E-05 | `include/*.h` | 🏗️ Estrutura | `[ ]` |
| E-06 | `src/Handlers/*.cpp` | 🏗️ Estrutura | `[ ]` |
| E-07 | `include/SystemStatus.h` | 🏗️ Estrutura | `[ ]` |
| E-08 | `src/Handlers/NextionHandler.cpp` | 🏗️ Estrutura | `[ ]` |
| E-09 | todos os headers | 🏗️ Estrutura | `[ ]` |
| E-10 | espalhado | 🏗️ Estrutura | `[ ]` |
| D-01 | `platformio.ini` | 🔴 Dep. Major | `[x]` |
| D-02 | `platformio.ini` | 🟢 Dep. Minor | `[x]` |
| D-03 | `platformio.ini` | 🟢 Dep. Minor | `[x]` |
| D-04 | `platformio.ini` | 🟡 Dep. Major | `[x]` |
| K-01 | `include/SystemStatus.h`, todos os `.cpp` | 🔴 Concorrência | `[ ]` |
| K-02 | `src/Handlers/DiagnosticsHandler.cpp`, `TaskHandler.cpp` | 🔴 Concorrência | `[x]` |
| K-03 | `src/Handlers/LogHandler.cpp` | 🔴 Concorrência | `[x]` |
| K-04 | `src/Handlers/OTAHandler.cpp` | 🔴 Crítico | `[x]` |
| K-05 | `src/Handlers/TemperatureControlHandler.cpp` | 🟠 Hardware | `[x]` |
| K-06 | `src/Handlers/TemperatureControlHandler.cpp` | 🟠 Hardware | `[x]` |
| K-07 | `src/Handlers/TemperatureControlHandler.cpp`, `main.cpp` | 🟠 Hardware | `[x]` |
| K-08 | `src/Handlers/LogHandler.cpp`, `MQTTHandler.cpp` | 🟡 Performance | `[x]` |
| K-09 | `src/Handlers/DiagnosticsHandler.cpp` | 🟡 Lógica | `[x]` |
| K-10 | `src/Handlers/LogHandler.cpp` | 🟡 Qualidade | `[x]` |

---

## Ordem de execução sugerida

```
Sprint 1 — Estabilidade crítica
  C-01  ✅ tasks duplicadas (race condition)
  C-02  ✅ MQTTHandler::begin() nunca chamado
  C-03  ✅ processMessage() não implementado
  C-04  ✅ initWiFi() nunca chamado
  C-18  ✅ duas instâncias de LogHandler
  C-29  ✅ client.loop() ausente no mqttTask
  C-30  conflito de page ID no Nextion (energia vs calibração)
  C-32  ✅ FileSystem.cpp duplicado — remover
  C-36  ✅ Serial.begin() ausente

Sprint 2 — Lógica e bugs ✅
  C-05  ✅ handle de task deletada no vetor
  C-06  ✅ isHealthy() bytes vs %
  C-08  ✅ OTA verifica partição errada
  C-09  ✅ histerese assimétrica
  C-10  ✅ MQTT task não inicia/para
  C-11  ✅ código morto (updateRelayState, pidControl, etc.)
  C-31  ✅ métodos não implementados em FileSystem
  C-37  ✅ timeout OTA nunca verificado
  C-39  ✅ -Wno-return-type mascara bugs (já estava comentado)

Sprint 3 — API, persistência e logging ✅
  C-12  ✅ idioma misto nas respostas — tudo em pt-BR
  C-13  ✅ senha MQTT mascarada ("***") no GET
  C-14  ✅ validação de range em bbqTemperature e proteinTemperature
  C-15  ✅ TempConfig valida min < max antes de salvar
  C-16  ✅ campo "status" duplicado removido do energy/cost
  C-17  ✅ MonitorEndpoints: bbqCurrentTemp/bbqSetpoint/proteinCurrentTemp/proteinSetpoint
  C-19  ✅ log não é mais apagado no boot; marcador "=== BOOT ===" adicionado
  C-20  ✅ logHandler.begin() chamado logo após LittleFS montar
  C-21  ✅ logMessage/logError/logRequest unificados via writeLog
  C-22  ✅ publishAllMessages() só loga erros de publicação
  C-23  ✅ senha MQTT removida do log de boot
  C-33  ✅ kWhCost persistido e carregado do config.json
  C-34  ✅ clientId MQTT usa deviceId (MAC) em vez de random

Sprint 4 — Qualidade e limpeza ✅
  C-24  ✅ getCurrentPageId() chamado 1x por loop, resultado passado às 4 funções
  C-25  ✅ já resolvido: ArduinoJson 7.x usa JsonDocument (sem StaticJsonDocument)
  C-26  ✅ checkTasks/logMetrics movidos para DiagTask FreeRTOS (60s)
  C-35  ✅ Nextion.h removido de 7 arquivos; dbSerial→Serial no LogHandler
  C-38  ✅ SD removida; EspSoftwareSerial mantida (dependência interna do Nextion)
  C-40  ✅ deviceId agora usado como MQTT clientId (C-34)
  C-41  ✅ diretórios src/Webhooks/ e include/Webhooks/ removidos
  C-42  ✅ README: ACS712→MAX6675 (x2) + DS18B20
  C-43  ✅ feature de energia removida completamente

Sprint 5 — Segurança e features pendentes ✅
  C-07  ✅ cureProcessMode removido (opção B: endpoint e flag removidos)
  C-27  ✅ aiKey default alterado de chave real para "" vazio
  C-28  ✅ API key via X-API-Key header; PATCH/POST protegidos; AuthEndpoints criado

Sprint 7 — Atualização de dependências (fazer em branch separado, testar no hardware)
  D-01  ✅ espressif32 6.10.0 → 7.0.0
  D-02  ✅ ESPAsyncWebServer-esphome 3.3.0 → 3.4.1
  D-03  ✅ ArduinoJson 7.3.0 → 7.4.3
  D-04  ✅ DallasTemperature 3.11.0 → 4.0.6

Sprint K — Concorrência, hardware e runtime (achados Kimi-K2.6) ✅ (parcial)
  K-04  ✅ Update.end(false) — rejeita firmware incompleto
  K-09  ✅ divisão por zero na fragmentação de heap
  K-05  ✅ DS18B20 valida -127 (desconectado) e 85 (power-on reset)
  K-06  ✅ MAX6675 valida NaN e valores fora de 0–500°C
  K-02  ✅ hardware WDT registra TempTask e ControlTask + reset periódico
  K-07  ✅ neopixelWrite centralizado no loop() com cache de estado
  K-03  ✅ mutex no LogHandler — buffer protegido contra escrita concorrente
  K-08  ✅ snprintf em buffer fixo char[256] no writeLog; char[40] no MQTT clientId
  K-01  [ ] mutex global para sysStat — adiado (maior invasividade)

Sprint 6 — Refatoração estrutural (fazer depois de tudo estabilizado)
  E-01  mover headers para subpastas include/Endpoints/ e include/Handlers/
  E-02  mover ResponseHelper.h para include/Endpoints/
  E-03  alinhar nomes .h / .cpp (TemperatureControl, WebServerControl, FileSystem)
  E-04  corrigir include guards de MonitorEndpoints.h e WebServerControl.h
  E-05  remover #include <AsyncTCP.h> desnecessários
  E-06  eliminar extern espalhados — passar dependências por parâmetro
  E-07  reorganizar SystemStatus em sub-structs semânticas
  E-08  dividir NextionHandler.cpp em Components + Handler
  E-09  padronizar sufixos Handler/Controller/Service
  E-10  criar include/Config.h com todas as constantes do sistema
```
