# Análise de Production Readiness — BBQ-Nextion

Data: 2026-05-08

---

## BUGS CRÍTICOS (impedem uso em produção)

### C-01 — `saveConfigToFile` não salva setpoints nem calibração ⚠️ BLOQUEANTE

`FileSystemHandler.cpp:saveConfigToFile()` salva apenas configurações de limites e credenciais.
**Não salva e não carrega:** `bbqTemperature`, `proteinTemperature`, `tempCalibration`, `tempCalibrationP`.

```cpp
// saveConfigToFile — campos ausentes:
// doc["bbqTemperature"]  = status.bbqTemperature;    ← NÃO EXISTE
// doc["proteinTemperature"] = status.proteinTemperature; ← NÃO EXISTE
// doc["tempCalibration"] = status.tempCalibration;   ← NÃO EXISTE
// doc["tempCalibrationP"] = status.tempCalibrationP; ← NÃO EXISTE
```

**Consequência:** Reboot perde sempre os setpoints e a calibração, independente de qualquer fix aplicado.
**Fix:** Adicionar os 4 campos em `saveConfigToFile()` e nos blocos de load/create de `initializeAndLoadConfig()`.

---

### C-02 — `resetSystem()` apaga calibração por engano

`TemperatureControlHandler.cpp:204`:
```cpp
sysStat.tempCalibration  = 0;   // ← apaga calibração junto com setpoints
sysStat.tempCalibrationP = 0;
```

`resetSystem()` deveria resetar os setpoints de temperatura (modo de operação), mas a calibração é uma configuração de hardware que não faz sentido zerar num reset de sessão.

**Fix:** Remover as linhas que zeram `tempCalibration` e `tempCalibrationP` do `resetSystem()`.

---

### C-03 — Buffer overflow no LogHandler quando flush falha

`LogHandler.cpp:135`:
```cpp
if (bufferIndex + (size_t)len >= LOG_BUFFER_SIZE) {
    flushBuffer();  // se LittleFS.open() falhar, retorna SEM limpar o buffer
}
memcpy(logBuffer + bufferIndex, msgBuf, len);  // ESCRITA APÓS O FIM DO BUFFER
bufferIndex += len;
```

`flushBuffer()` retorna silenciosamente se o arquivo não abrir. Na sequência, `memcpy` escreve além dos 1024 bytes do `logBuffer`. Em sistema embarcado, isso corrompe memória adjacente.

**Fix:** Após `flushBuffer()` falhar, descartar as mensagens mais antigas para liberar espaço:
```cpp
if (bufferIndex + (size_t)len >= LOG_BUFFER_SIZE) {
    flushBuffer();
    if (bufferIndex + (size_t)len >= LOG_BUFFER_SIZE) {
        // flush falhou — descarta buffer para não corromper memória
        bufferIndex = 0;
    }
}
```

---

### C-04 — Histerese do relé sem banda morta

`TemperatureControlHandler.cpp:143`:
```cpp
if (temp >= sysStat.bbqTemperature) {
    relay OFF;
}
else if (temp <= sysStat.bbqTemperature + TEMP_HYSTERESIS) {
    relay ON;   // ← sempre verdadeiro quando temp < setpoint
}
```

A condição `else if` é verdadeira em QUALQUER temperatura abaixo do setpoint (pois HYSTERESIS é positivo). Não há banda morta — o relé oscila a cada leitura ao redor do setpoint. Histerese real:
- OFF quando `temp >= setpoint`
- ON quando `temp < setpoint - HYSTERESIS`
- Mantém estado atual entre os dois limiares

**Fix:**
```cpp
if (temp >= sysStat.bbqTemperature) {
    digitalWrite(RELAY_PIN, LOW);
    sysStat.isRelayOn = false;
}
else if (temp < sysStat.bbqTemperature - TEMP_HYSTERESIS) {
    digitalWrite(RELAY_PIN, HIGH);
    sysStat.isRelayOn = true;
}
// entre (setpoint-2) e setpoint: mantém estado atual
```

---

## BUGS ALTOS (degradam confiabilidade)

### A-01 — `isHealthy()` chamado em todo loop sem throttle

`main.cpp:108`:
```cpp
void loop() {
    // ...
    if (!diagnostics.isHealthy()) {   // ← executa a cada iteração do loop (~10-40x/s)
        logHandler.logError("Sistema com recursos críticos!");
    }
}
```

`isHealthy()` chama `getMetrics()` que invoca `esp_get_free_heap_size()`, `esp_get_minimum_free_heap_size()`, `heap_caps_get_largest_free_block()`. Se o sistema estiver degradado, gera erro de log 40x por segundo, acelerando a fragmentação do heap e desgastando LittleFS.

**Fix:** Rate-limit de 60 segundos:
```cpp
static unsigned long lastHealthCheck = 0;
unsigned long now = millis();
if (now - lastHealthCheck >= 60000) {
    lastHealthCheck = now;
    if (!diagnostics.isHealthy()) {
        logHandler.logError("Sistema com recursos críticos!");
    }
}
```

---

### A-02 — Endpoint de log carrega arquivo inteiro em RAM

`GeneralEndpoints.cpp:18`:
```cpp
std::vector<String> lines;
while (logFile.available()) {
    lines.push_back(logFile.readStringUntil('\n'));  // aloca cada linha na heap
}
// depois concatena tudo em uma String (O(n²))
String logContent = "";
for (int i = startLine; i < lines.size(); i++) {
    logContent += lines[i] + '\n';
}
```

Com log de 50KB configurado, isso aloca o arquivo inteiro mais um `vector<String>` com centenas de objetos. Em ESP32 com ~180KB de heap livre, pode causar OOM e fragmentação severa.

**Fix:** Ler direto do final do arquivo:
```cpp
File logFile = LittleFS.open("/log.txt", "r");
size_t fileSize = logFile.size();
if (fileSize > 5000) logFile.seek(fileSize - 5000);
String logContent = logFile.readString();
logFile.close();
```

---

### A-03 — DebugInjector: race condition entre web task e TempTask

`DebugInjector.h:16`:
```cpp
inline bool debugInjectorIsActive() {
    if (debugInjector.active && millis() >= debugInjector.endTimeMs) {
        debugInjector.active = false;   // ← escrita sem mutex
    }
    return debugInjector.active;
}
```

`TempTask` (Core 0) lê e escreve `debugInjector.active` via esta função. O servidor web (Core 1) escreve via endpoint POST. Em ESP32 dual-core, escritas simultâneas em `bool` não são garantidamente atômicas entre cores sem barreira de memória.

**Fix:** Marcar o campo como `volatile` e proteger via `sysStatLock()` no endpoint, ou incluir `debugInjector` dentro do `sysStat` com proteção natural do mutex.

---

### A-04 — Config.json corrompido: sem fallback para defaults

`FileSystemHandler.cpp:83`:
```cpp
auto error = deserializeJson(doc, buf.get());
if (error) {
    logHandler.logMessage("Falha ao deserializeJson config.json!");
    return;   // ← retorna com sysStat parcialmente não inicializado
}
```

Se o JSON estiver corrompido (write incompleto, flash failure), o sistema inicia com `sysStat` zerado e não busca o backup `config.bak.json` já existente.

**Fix:** Tentar o backup antes de desistir:
```cpp
if (error) {
    logHandler.logError("config.json corrompido, tentando backup...");
    configFile.close();
    configFile = LittleFS.open("/config.bak.json", "r");
    if (configFile) {
        // re-tenta deserialize do backup
    } else {
        logHandler.logError("Sem backup — usando valores padrão");
        applyDefaults(status);
    }
}
```

---

## PROBLEMAS MODERADOS (qualidade e robustez)

### M-01 — Config.json sem limite de tamanho antes de alocar

`FileSystemHandler.cpp:75`:
```cpp
size_t size = configFile.size();
std::unique_ptr<char[]> buf(new char[size + 1]);   // sem limite
```

Se o arquivo for corrompido com tamanho falso, aloca memória arbitrária.

**Fix:** `if (size > 8192) { logError(...); return; }`

---

### M-02 — GET endpoints lêem `sysStat` sem mutex (campos `char[]`)

A maioria dos endpoints GET lê `systemStatus` sem `sysStatLock()`. Para campos `int` e `bool`, reads de 32-bit no ARM são atômicos. Mas para `char mqttServer[100]`, `char aiKey[128]`, etc., uma leitura de 100 bytes enquanto `TempTask` ou `MQTTTask` escreve pode retornar dados misturados.

**Fix:** No início de cada GET handler que lê char arrays, fazer um snapshot:
```cpp
// Snapshot thread-safe
char mqttServer[100];
sysStatLock();
strlcpy(mqttServer, systemStatus.mqttServer, sizeof(mqttServer));
sysStatUnlock();
```

---

### M-03 — `handleTaskTimeout` apaga task crítica antes de reiniciar

`DiagnosticsHandler.cpp:70`:
```cpp
void DiagnosticsHandler::handleTaskTimeout(const TaskInfo& task) {
    vTaskDelete(task.handle);   // ← deleta TempTask ou ControlTask
    if (...TempTask || ControlTask...) {
        ESP.restart();          // ← reinicia logo depois
    }
}
```

Entre `vTaskDelete()` e `ESP.restart()`, o sistema fica sem controle de temperatura. Para tasks críticas, ir direto para `ESP.restart()` sem deletar é mais seguro.

---

### M-04 — `saveConfigToFile` não retorna sucesso/falha para o chamador

`FileSystem::saveConfigToFile()` é `void`. Endpoints que chamam a função assumem sucesso e retornam 200 para o cliente, mas o save pode ter falhado silenciosamente.

**Fix:** Mudar para `bool saveConfigToFile(...)` e retornar 500 em falha.

---

### M-05 — `METRICS_INTERVAL` não definido em Config.h

`DiagnosticsHandler.cpp:91` usa `METRICS_INTERVAL` que não está em `Config.h`. Está provavelmente definido no header de DiagnosticsHandler. Para consistência, centralizar em `Config.h`.

---

### M-06 — `calculateAverage()` loga cada cálculo de média

`TemperatureControlHandler.cpp:183`:
```cpp
logHandler.logMessage("Average Temp: " + String(sysStat.averageTemp));
```

Esta função é chamada a cada minuto — log OK. Mas se `SOFT_WDT_INTERVAL` for ajustado, pode ser chamada com mais frequência, enchendo o log com dados de baixo valor.

---

## CHECKLIST DE PRODUCTION READINESS

| Item | Status | Prioridade |
|------|--------|-----------|
| Setpoints e calibração persistidos | ❌ C-01 | CRÍTICO |
| Reset não apaga calibração | ❌ C-02 | CRÍTICO |
| LogHandler sem buffer overflow | ❌ C-03 | CRÍTICO |
| Histerese com banda morta real | ❌ C-04 | CRÍTICO |
| isHealthy() com rate-limit | ❌ A-01 | Alto |
| Log endpoint sem OOM | ❌ A-02 | Alto |
| DebugInjector thread-safe | ❌ A-03 | Alto |
| Fallback para config.bak.json | ❌ A-04 | Alto |
| Config.json com limite de tamanho | ❌ M-01 | Moderado |
| GET endpoints com snapshot de char[] | ❌ M-02 | Moderado |
| Watchdog antes de vTaskDelete | ❌ M-03 | Moderado |
| saveConfigToFile retorna bool | ❌ M-04 | Moderado |
| METRICS_INTERVAL em Config.h | ❌ M-05 | Baixo |
| NTP configurado (sem WiFi, log usa millis()) | ⚠️ Sem NTP | Baixo |
| Mutex inicializado antes das tasks | ✅ OK na ordem atual | — |
| Failsafe de temperatura (MAX_SAFE_TEMP) | ✅ Implementado | — |
| Backup automático do config.json | ✅ Implementado | — |
| Hardware watchdog ativo | ✅ Implementado | — |
| OTA com verificação de MD5 | ✅ Implementado | — |
| Lock do sysStat nas tasks | ✅ Implementado | — |
| Cache de leitura dos sensores | ✅ Implementado | — |

---

## NOTA: Achados do agente que NÃO são bugs

- **"getCalibratedTemp sem lock"** — INCORRETO: o lock está na `temperatureTask` em TaskHandler.cpp.
- **"Mutex não inicializado antes das tasks"** — INOFENSIVO: tasks são criadas em `initializeTasks()` chamado após `xSemaphoreCreateRecursiveMutex()`.
- **"DiagnosticsHandler: handle inválido após delete"** — INOFENSIVO: `checkTasks()` chama `erase()` imediatamente após o delete; o handle nunca é reutilizado.
- **"CORS muito permissiva"** — ACEITÁVEL: dispositivo de rede local, não exposto à internet.
