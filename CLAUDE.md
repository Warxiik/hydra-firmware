# Hydra Firmware

Custom firmware for the **Hydra dual-nozzle 3D printer** — a CoreXY printer where two nozzles print simultaneously on the same layer, achieving ~2x print speed.

## Project Structure

```
hydra-firmware/
├── mcu/          # RP2040 real-time firmware (C, Pico SDK)
├── host/         # Raspberry Pi CM5 host software (C++20, CMake)
├── ui/           # Touchscreen display UI (Qt6/QML)
├── shared/       # Protocol definitions shared between host + MCU
├── hardware/     # Carrier board KiCad files, BOM, enclosure
├── docs/         # Architecture, protocol spec, build guide
├── config/       # Machine config (TOML), systemd services
├── tools/        # Flash scripts, cross-compile toolchain, deploy
├── scripts/      # Dev setup, test runner
└── .github/      # CI workflows
```

## Hardware Platform

| Component | Choice | Notes |
|-----------|--------|-------|
| SBC | Raspberry Pi CM5 (4GB, WiFi, 32GB eMMC) | The brain — runs host software + UI |
| MCU | RP2040 | The muscles — real-time step generation via PIO |
| Display | Pi Touch Display 2 (7" 720p IPS, DSI) | Capacitive touch, Qt6/QML UI |
| Stepper drivers | TMC2209 x7 (UART config via PIO) | StealthChop, sensorless homing |
| Step mux | 74HC595 shift register | PIO drives 7 axes from 3 GPIO pins |
| PSU | 24V / 500W (Mean Well RSP-500-24) | Powers everything |
| Carrier board | Custom PCB (KiCad) | CM5 socket + RP2040 + TMC2209s + power |

## Architecture: Klipper-Style Split

```
CM5 (Linux)                          RP2040 (bare metal)
┌─────────────────────┐              ┌─────────────────────┐
│ Dual G-code parser  │              │ PIO step generation │
│ Move planner x2     │    SPI 4MHz  │ ADC thermistors     │
│ Barrier sync engine │ ──────────── │ Heater/fan PWM      │
│ CoreXY kinematics   │   protocol   │ Endstop monitoring  │
│ Thermal PID         │              │ TMC2209 UART (PIO)  │
│ Qt6/QML UI          │              │ Watchdog / e-stop   │
│ Web dashboard       │              │ Shift-reg step out  │
└─────────────────────┘              └─────────────────────┘
```

All "smart" logic runs on the CM5. The RP2040 only does real-time I/O.

## Companion Repos

- **HydraSlicer** (`C:/Users/nikol/3D!/hydra-slicer/`) — Python slicer that generates dual G-code streams with sync markers
- **Hydra Web** (TBD) — Marketing site, cloud platform, remote monitoring

## Sync Protocol (from slicer)

The slicer outputs two `.gcode` files (one per nozzle) with these markers:

```gcode
; SYNC BARRIER <id>              — both nozzles must reach before proceeding
; TASK BEGIN <id> nozzle=<n> layer=<l>  — task start
; TASK END <id>                  — task complete
; WAIT TASK <id> ; nozzle <n> layer <l> — wait for dependency
```

## Build Commands

```bash
# Host software (C++20)
cmake -B build/host -S host && cmake --build build/host

# MCU firmware (requires Pico SDK)
cmake -B build/mcu -S mcu && cmake --build build/mcu

# UI (requires Qt6)
cmake -B build/ui -S ui && cmake --build build/ui

# Run tests
cmake --build build/host --target test

# Flash MCU
tools/flash-mcu.sh build/mcu/hydra-mcu.uf2

# Deploy to CM5
tools/deploy-host.sh <cm5-ip>
```

## Conventions

- **Host code**: C++20, snake_case, `.cpp`/`.h` extensions, `clang-format` with project `.clang-format`
- **MCU code**: C11, snake_case, `.c`/`.h` extensions
- **QML**: PascalCase for types, camelCase for properties, 4-space indent
- **Commits**: conventional commits (`feat:`, `fix:`, `docs:`, `hw:`, `mcu:`, `host:`, `ui:`)
- **Includes**: `#include "hydra/..."` for project headers, `<>` for system/library headers
- **Error handling**: Host uses exceptions at boundaries, `std::expected` internally. MCU uses error codes.
- **No dynamic allocation on MCU** — all buffers are statically sized
- **Tests**: GoogleTest for host, minimal assert-based tests for MCU protocol logic

## Stepper Channels

| Channel | Motor | Function |
|---------|-------|----------|
| 0 | CoreXY A | Shared frame motor A (X+Y) |
| 1 | CoreXY B | Shared frame motor B (X-Y) |
| 2 | Offset X | Nozzle 1 relative X offset |
| 3 | Offset Y | Nozzle 1 relative Y offset |
| 4 | Z | Bed Z axis |
| 5 | Extruder 0 | Nozzle 0 filament drive |
| 6 | Extruder 1 | Nozzle 1 filament drive |

## Key Design Decisions

1. **Why custom firmware, not Klipper?** — No existing firmware supports parallel G-code streams for simultaneous multi-nozzle printing
2. **Why SPI not USB for host↔MCU?** — Deterministic latency, no USB stack jitter, simpler on custom carrier board
3. **Why RP2040 not STM32?** — PIO state machines give hardware-level step timing precision; $1 cost; shift-register approach fits 7 axes
4. **Why Qt6/QML not web UI?** — Hardware-accelerated 60fps touch UI via EGLFS; professional look for Kickstarter product
5. **Why single RP2040?** — Shift-register PIO uses only 2-3 state machines for all 7 steppers; plenty of headroom
