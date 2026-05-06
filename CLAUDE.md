# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BBQ-Nextion is an ESP32-S3 firmware for controlling a pit smoker. It reads temperatures from two MAX6675 thermocouples (pit chamber + protein probe), controls a relay to manage heat, and exposes a REST API served from LittleFS. A Nextion touchscreen display provides local UI; a React SPA in `data/` provides web UI.

## Build & Flash Commands

This project uses PlatformIO. All commands target the `esp32-s3-devkitc-1-n16r8` environment.

```bash
pio run                          # compile firmware
pio run --target upload          # compile + flash firmware
pio run --target uploadfs        # build + flash LittleFS filesystem (web UI)
pio device monitor               # serial monitor at 115200 baud
pio run --target upload && pio run --target uploadfs  # full flash
```

There are no automated tests — `test/` is empty.

## Architecture

### Central State (`include/SystemStatus.h`)

`SystemStatus` is a plain struct passed by reference to every subsystem. It holds all runtime state: calibrated temperatures, relay state, BBQ/protein temperature setpoints, calibration offsets, MQTT credentials, AI config, and energy monitoring values. There are no getters/setters — fields are read/written directly.

### Execution Model (`src/main.cpp`)

Global instances are created in `main.cpp` and passed by reference into handlers/controllers. FreeRTOS tasks run in parallel:
- **TempTask** (Core 1, priority 2, 500ms): reads both MAX6675 sensors and DS18B20
- **ControlTask** (Core 0, priority 1, 1s): relay control with 2°C hysteresis
- **MQTTTask** (Core 1, priority 1, 3s): publishes to Home Assistant — only created if `sysStat.isHAAvailable`

The Arduino `loop()` drives the Nextion event loop (`nexLoop`) and calls update functions for each display page, feeding the watchdog each iteration.

### Layers

| Layer | Location | Purpose |
|---|---|---|
| Handlers | `src/Handlers/` | Hardware drivers: temperature sensors, Nextion display, MQTT, WiFi, OTA, filesystem, task management, diagnostics |
| Endpoints | `src/Endpoints/` | REST API registration using ESPAsyncWebServer |
| Headers | `include/` | Declarations mirroring `src/` structure |
| Web UI | `data/` | React SPA (pre-built) stored in LittleFS |

### REST API

`WebServerControl::begin()` registers all endpoint groups before configuring static file serving (order matters — routes must come before `serveStatic`). All responses go through `ResponseHelper` in `src/Endpoints/ResponseHelper.h`, which adds CORS headers. API prefix: `/api/v1/`.

Key endpoint groups: `temperature`, `mqtt-config`, `temp-config`, `energy`, `ai`, `system`, `monitor`, `diagnostics`, `general`.

### Nextion Display

Connected on `Serial2` (RX=16, TX=17, 9600 baud). Pages are polled each loop via `getCurrentPageId()` and display variables are only written when their values change (cache in `nexCache`). Button callbacks write directly into `sysStat`. Nextion page IDs: 0=wifi, 1=welcome, 2=menu, 3=monitor, 4=BBQTemp, 5=ChunkTemp, 6=calibration, 7=ap, 8=init.

### Persistence

`FileSystem` stores config as `/config.json` on LittleFS, with automatic backup to `/config.bak.json`. `FileSystem::initializeAndLoadConfig()` is called once at startup; `FileSystem::saveConfigToFile()` is called on any config change endpoint.

## Key Hardware

| Symbol | Pin | Purpose |
|---|---|---|
| `RELAY_PIN` | 1 | Relay output (HIGH = on) |
| `MAX6675_SCK/CS/SO` | 12/13/14 | BBQ thermocouple |
| `MAX6675_SCK_P/CS_P/SO_P` | 9/10/11 | Protein probe thermocouple |
| `Ds18b2` | 4 | DS18B20 internal temperature |
| Serial2 RX/TX | 16/17 | Nextion display |
| `RGB_BUILTIN` | — | Red = relay on, Blue = relay off |

## Adding a New Endpoint

1. Create `src/Endpoints/MyEndpoints.cpp` and `include/MyEndpoints.h`
2. Implement `void registerMyEndpoints(AsyncWebServer&, SystemStatus&, LogHandler&)` using `ResponseHelper` for all responses
3. Call `registerMyEndpoints(...)` inside `WebServerControl::begin()` before the `serveStatic` calls
