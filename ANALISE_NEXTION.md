# Análise Profunda — Nextion Handler

## Visão Geral da Arquitetura Atual

O fluxo de execução Nextion ocorre inteiramente no Arduino `loop()`:

```
loop()
  ├─ nexLoop(nex_listen_list)        → processa touch events (biblioteca Nextion)
  ├─ getCurrentPageId()              → envia "sendme" e aguarda resposta 100ms
  ├─ sysStatLock()
  │    ├─ updateNextionMonitor...    → escreve no display via Serial2
  │    ├─ updateNextionSetBBQ...     → escreve no display via Serial2
  │    ├─ updateNextionSetChunk...   → escreve no display via Serial2
  │    └─ updateNextionSetCali...    → escreve no display via Serial2
  └─ sysStatUnlock()
```

---

## Bugs (Impacto Real em Produção)

### B-01 — Ausência de Persistência nas Callbacks do Nextion ⚠️ CRÍTICO

**Problema:** Quando o usuário seta temperatura ou calibração pelo touchscreen, os valores são escritos em `sysStat` mas `fileSystem.saveConfigToFile()` **nunca é chamado**. Se o dispositivo reiniciar, todos os valores voltam ao que estava em `config.json`.

A REST API salva corretamente:
```cpp
// TempConfigEndpoints.cpp:73
fileSystem.saveConfigToFile(systemStatus);  // ← REST API faz isso
```

As callbacks Nextion não:
```cpp
// setBBQTempPushCallback — NÃO PERSISTE
sysStatLock();
systemStatus->bbqTemperature = static_cast<int>(value);
sysStatUnlock();
monitor.show();
// ← fileSystem.saveConfigToFile() deveria estar aqui
```

**Afeta:** `setBBQTempPushCallback`, `setChunkTempPushCallback`, `setCaliPushCallback`.

**Fix:** Declarar `extern FileSystem fileSystem;` em `NextionHandler.cpp` (mesmo padrão do `extern LogHandler logHandler`) e chamar `fileSystem.saveConfigToFile(sysStat)` após cada escrita.

---

### B-02 — `delay(100)` Bloqueante em Cada Iteração do Loop

**Problema:** `getCurrentPageId()` envia "sendme" e espera 100ms incondicionalmente:

```cpp
nexSerial.print("sendme");
nexSerial.write(0xff); nexSerial.write(0xff); nexSerial.write(0xff);
delay(100);  // ← bloqueia 100ms TODA iteração do loop()
```

A 9600 baud, 5 bytes chegam em ~5ms. O `delay(100)` é 20x maior que o necessário.

**Consequência:** O `loop()` roda no máximo 10x/segundo. Isso impacta:
- Responsividade dos botões Nextion (`nexLoop` fica 100ms parado entre chamadas)
- Todos os outros processamentos de `loop()`

**Fix:** Reduzir para `delay(25)` — margem confortável para 9600 baud com processamento do display.

---

### B-03 — Comunicação Serial Dentro do Mutex

**Problema:** O `loop()` chama as 4 funções de update dentro de `sysStatLock()`:

```cpp
sysStatLock();
updateNextionMonitorVariables(...);   // faz setValue() → Serial2 write
updateNextionSetBBQVariables(...);    // faz setValue() → Serial2 write
...
sysStatUnlock();
```

`setValue()` é uma operação de I/O serial síncrona a 9600 baud. Enquanto o mutex está tomado, `TempTask` e `ControlTask` ficam bloqueadas esperando para ler `sysStat`. Isso cria jitter desnecessário nas leituras de temperatura.

**Fix:** Copiar os valores necessários para variáveis locais dentro do mutex, liberar o mutex, depois escrever no display:

```cpp
// Padrão correto
sysStatLock();
int bbqTemp = sysStat.calibratedTemp;
int bbqTarget = sysStat.bbqTemperature;
// ...copia todos os campos necessários
sysStatUnlock();

// Escreve no display FORA do mutex
if (bbqTemp != nexCache.lastBBQTemp) {
    bbqTempComp.setValue(bbqTemp);
    nexCache.lastBBQTemp = bbqTemp;
}
```

---

### B-04 — Página `energyPg` no HMI sem Handler

**Problema:** O arquivo `tela.HMI` contém navegação para `page energyPg`, mas não existe `NEXTION_PAGE_ENERGY` em `Config.h` nem função `updateNextionSetEnergyVariables()`. Quando o usuário navega para essa página, o firmware não sabe em qual página está (retorna `0xFF`) e não atualiza nada.

---

## Fragilidades de Design

### D-01 — `nexLoop` e `getCurrentPageId` Compartilham Serial2 sem Flush

`nexLoop()` processa a Serial2 internamente. Logo após, `getCurrentPageId()` envia "sendme" e lê a resposta — mas se `nexLoop()` deixou bytes não consumidos no buffer, eles serão lidos antes do `0x66` esperado, e a função retornará `0xFF` (inválido).

O código faz flush **só** antes de `setPageBackground()`, não antes de `sendme`. Na prática funciona porque `nexLoop()` tende a consumir tudo, mas é frágil.

**Fix:** Adicionar flush antes do "sendme":
```cpp
while (nexSerial.available()) nexSerial.read(); // flush
nexSerial.print("sendme");
```

---

### D-02 — Sem Validação de Range nas Callbacks

`setBBQTempPushCallback` aceita qualquer valor que o display enviar sem validar o range (30–250°C). O display limita o input, mas a ausência de validação no firmware é falta de defesa em profundidade.

A lógica de range já existe no MQTT handler. Deveria ser compartilhada.

---

### D-03 — Magic Numbers para Background do Relay

```cpp
setPageBackground("monitor", sysStat.isRelayOn ? 4 : 1);
```

Os IDs de imagem `4` e `1` são números mágicos sem significado óbvio. Qualquer mudança no HMI exige caçar esses valores no código.

**Fix:** Adicionar em `Config.h`:
```cpp
#define NEXTION_BG_RELAY_ON  4
#define NEXTION_BG_RELAY_OFF 1
```

---

### D-04 — `uint32_t lastPageId = -1` como Sentinela

```cpp
static uint32_t lastPageIdBBQ = -1;  // converte para 0xFFFFFFFF
```

Funciona, mas mistura tipo sem sinal com valor negativo. Mais claro seria:
```cpp
static uint32_t lastPageIdBBQ = 0xFF;  // "nenhuma página" explícito
```

---

### D-05 — Estado Duplicado: `initialUpdateDone*` + `forceUpdate`

Cada página tem dois mecanismos de estado ao entrar:
1. `forceUpdate` — força atualização dos campos estáticos (min/max)  
2. `initialUpdateDone*` — inicializa o campo editável com o valor atual

São necessários porque servem a campos diferentes, mas a lógica espalhada em 3 variáveis por página (`lastPageId*`, `initialUpdateDone*`, `forceUpdate` local) é difícil de seguir.

---

### D-06 — `nexCache.lastUpdate` Throttle Afeta Só o Monitor

O throttle de 500ms em `updateNextionMonitorVariables` atualiza `nexCache.lastUpdate` mesmo quando a página não é Monitor. Isso é inofensivo mas confuso — parece throttle global mas só afeta a função Monitor.

As páginas BBQ/Chunk/Cali não têm throttle de tempo — dependem apenas da detecção de mudança de valor (suficiente já que o loop é lento).

---

## Pontos Positivos (O Que Funciona Bem)

- **Cache de valores por campo**: Cada campo tem seu `last*` — só escreve no display quando o valor muda. Economiza comunicação serial.
- **Reset de `initialUpdateDone*` ao sair da página**: Quando o usuário volta a uma página, o valor atual é re-enviado ao display. Correto.
- **Callbacks tomam o mutex individualmente**: `nexLoop()` roda fora do mutex principal; os callbacks tomam o lock só quando necessário. Padrão correto para mutex recursivo.
- **`forceUpdate` na entrada de página**: Garante que min/max sejam enviados ao display na primeira renderização da página.
- **Buffer de 256 bytes em `Serial2.begin()`**: Evita perda de eventos em bursts de comunicação.

---

## Plano de Correções (por prioridade)

| ID | Impacto | Esforço | Descrição |
|----|---------|---------|-----------|
| B-01 | 🔴 Alto | Baixo | Persistir setpoints e calibração nas callbacks Nextion |
| B-02 | 🟠 Médio | Baixo | Reduzir `delay(100)` para `delay(25)` |
| B-03 | 🟠 Médio | Médio | Copiar sysStat para locais, liberar mutex, escrever no display |
| B-04 | 🟡 Baixo | Médio | Adicionar `NEXTION_PAGE_ENERGY` e handler (ou remover do HMI) |
| D-01 | 🟡 Baixo | Baixo | Flush antes de "sendme" |
| D-02 | 🟡 Baixo | Baixo | Validar range 30–250 em setBBQTempPushCallback |
| D-03 | 🟢 Info | Baixo | Constantes para IDs de background no Config.h |
| D-04 | 🟢 Info | Baixo | Trocar `= -1` por `= 0xFF` nos lastPageId* |

---

## O Que Não Vale Mudar

- **Comunicação serial a 9600 baud**: Padrão do hardware Nextion — não é configurável no código.
- **`nexLoop()` no Arduino `loop()`**: É o modelo de uso correto da biblioteca Nextion; não mover para task FreeRTOS (a biblioteca usa variáveis globais não reentrantes).
- **Polling com "sendme"**: Sem mudanças no HMI para habilitar eventos de mudança de página automáticos, polling é a única opção.
