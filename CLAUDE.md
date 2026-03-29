# Hydra Firmware

Custom firmware for the **Hydra multi-nozzle valve array 3D printer** — a CoreXY printer with a shared melt chamber and 4 solenoid-controlled needle valve nozzles for variable-width extrusion.

## Project Structure

```
hydra-firmware/
├── mcu/          # RP2040 real-time firmware (C, Pico SDK)
├── host/         # Raspberry Pi CM5 host software (C++20, CMake)
├── ui/           # Web UI (React + TypeScript + Tailwind CSS)
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
| Display | Pi Touch Display 2 (7" 720p IPS, DSI) | Capacitive touch, Chromium kiosk |
| Stepper drivers | TMC2209 x7 (UART config via PIO) | StealthChop, sensorless homing |
| Step mux | 74HC595 shift register | PIO drives 7 axes from 3 GPIO pins |
| PSU | 24V / 500W (Mean Well RSP-500-24) | Powers everything |
| Carrier board | Custom PCB (KiCad) | CM5 socket + RP2040 + TMC2209s + power |
| Valve actuators | 4x 24V solenoid + IRLZ34N MOSFET | Needle valve nozzle selection |

## Architecture: Multi-Nozzle Valve Array

```
Single filament → BMG extruder → shared melt chamber (manifold block)
                                    ↓
                    ┌───────────────┼───────────────┐
                    ↓               ↓               ↓               ↓
              Valve 0 (0.4mm)  Valve 1 (0.6mm)  Valve 2 (0.6mm)  Valve 3 (0.8mm)
              quality nozzle   infill nozzle     infill nozzle     infill nozzle

Valve control: M600 V<bitmask>  (e.g. M600 V1 = quality, M600 V14 = 3x infill, M600 V15 = all)
Extruder speed scales by number of open nozzles.
```

## Klipper-Style Split

```
CM5 (Linux)                          RP2040 (bare metal)
┌─────────────────────┐              ┌─────────────────────┐
│ G-code parser       │              │ PIO step generation │
│ Move planner        │    SPI 4MHz  │ ADC thermistors     │
│ CoreXY kinematics   │ ──────────── │ Heater/fan PWM      │
│ Valve sync          │   protocol   │ Valve GPIO control  │
│ Thermal PID         │              │ Endstop monitoring  │
│ React web UI        │              │ TMC2209 UART (PIO)  │
│ WebSocket server    │              │ Watchdog / e-stop   │
└─────────────────────┘              └─────────────────────┘
```

All "smart" logic runs on the CM5. The RP2040 only does real-time I/O.

## Companion Repos

- **HydraSlicer** (`C:/Users/nikol/3D!/hydra-slicer/`) — Python slicer that generates G-code with M600 V<mask> valve commands
- **Hydra Web** (TBD) — Marketing site, cloud platform, remote monitoring

## Build Commands

```bash
# Host software (C++20)
cmake -B build/host -S host && cmake --build build/host

# MCU firmware (requires Pico SDK)
cmake -B build/mcu -S mcu && cmake --build build/mcu

# UI (React + Vite)
cd ui && npm install && npm run build

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
- **UI code**: React + TypeScript, Tailwind CSS v4, 2-space indent
- **Commits**: conventional commits (`feat:`, `fix:`, `docs:`, `hw:`, `mcu:`, `host:`, `ui:`)
- **Includes**: `#include "hydra/..."` for project headers, `<>` for system/library headers
- **Error handling**: Host uses exceptions at boundaries, `std::expected` internally. MCU uses error codes.
- **No dynamic allocation on MCU** — all buffers are statically sized
- **Tests**: GoogleTest for host, minimal assert-based tests for MCU protocol logic

## Stepper Channels

| Channel | Motor | Function |
|---------|-------|----------|
| 0 | CoreXY A | Frame motor A (X+Y) |
| 1 | CoreXY B | Frame motor B (X-Y) |
| 2 | Z1 | Bed Z lead screw 1 |
| 3 | Z2 | Bed Z lead screw 2 |
| 4 | Z3 | Bed Z lead screw 3 |
| 5 | Extruder | Shared filament drive |
| 6 | Spare | Future use |

## Valve Nozzles

| Valve | GPIO | Nozzle | Use |
|-------|------|--------|-----|
| 0 | GP13 | 0.4mm | Quality / detail |
| 1 | GP16 | 0.6mm | Infill / support |
| 2 | GP18 | 0.6mm | Infill / support |
| 3 | GP21 | 0.8mm | Infill / support |

Bitmask control: `0b0001` = quality, `0b1110` = 3x infill, `0b1111` = all open

## Key Design Decisions

1. **Why multi-nozzle valve array?** — Variable extrusion width without tool changes; 1 filament path, 4 outlets; truly novel approach
2. **Why needle valves?** — Precise flow control, fast actuation (~5ms), no drool/stringing between switches
3. **Why custom firmware, not Klipper?** — No existing firmware supports valve-synchronized multi-nozzle printing
4. **Why SPI not USB for host↔MCU?** — Deterministic latency, no USB stack jitter, simpler on custom carrier board
5. **Why RP2040 not STM32?** — PIO state machines give hardware-level step timing precision; $1 cost; shift-register approach fits 7 axes
6. **Why web UI (React)?** — Same UI on touchscreen (Chromium kiosk) and remote browser; single codebase; hot-reload dev
7. **Why 3x Z lead screws?** — Auto bed leveling via independent Z motors (replacing single Z + offset actuators)
