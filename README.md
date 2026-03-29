# Hydra Firmware

Custom firmware for the **Hydra dual-nozzle 3D printer** — two nozzles printing simultaneously, same quality, half the print time.

## Architecture

```
Raspberry Pi CM5 (Linux)              RP2040 (bare metal)
┌───────────────────────┐              ┌──────────────────────┐
│ Dual G-code parser    │              │ PIO step generation  │
│ Move planner x2       │   SPI 4MHz   │ 74HC595 shift reg    │
│ Sync barrier engine   │ ◄──────────► │ ADC thermistors      │
│ CoreXY kinematics     │   protocol   │ Heater/fan PWM       │
│ Thermal PID           │              │ TMC2209 UART (PIO)   │
│ React web UI (kiosk)  │              │ Endstop monitoring   │
│ WebSocket server      │              │ Watchdog / e-stop    │
└───────────────────────┘              └──────────────────────┘
```

## Hardware

| Component | Spec |
|-----------|------|
| Controller | Raspberry Pi CM5 (4GB RAM, WiFi, 32GB eMMC) |
| Real-time MCU | RP2040 (dual-core Cortex-M0+, PIO) |
| Display | 7" IPS 720p capacitive touch (DSI) |
| Stepper drivers | 7x TMC2209 (UART, StealthChop) |
| Motion | CoreXY + nozzle offset actuators |
| Build volume | 220 x 220 x 250 mm |

## Building

### Host software (CM5)
```bash
cmake -B build/host -S host
cmake --build build/host
```

### MCU firmware (RP2040)
```bash
# Requires Pico SDK
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -B build/mcu -S mcu
cmake --build build/mcu
# Flash: copy build/mcu/hydra-mcu.uf2 to RP2040
```

### UI (React + Vite)
```bash
cd ui
npm install
npm run build
# Output: ui/dist/ (served by host at http://<ip>:5000/)
```

## Project Structure

```
mcu/          RP2040 firmware (C, Pico SDK)
host/         CM5 host software (C++20)
ui/           Web UI (React + TypeScript + Tailwind CSS)
shared/       Protocol definitions (C, shared between host+MCU)
hardware/     Carrier board PCB (KiCad), BOM
config/       Machine configuration, systemd services
docs/         Architecture and protocol documentation
```

## Companion Projects

- [HydraSlicer](https://github.com/Warxiik/hydra-slicer) — Python slicer with dual-nozzle support
- Hydra Web (TBD) — Cloud platform and remote monitoring

## License

MIT
