# Architecture

## Overview

Hydra uses a split architecture inspired by Klipper:

- **Host (Raspberry Pi CM5)**: All "smart" logic — G-code parsing, motion planning, valve synchronization, thermal control, UI
- **MCU (RP2040)**: Real-time I/O — step pulse generation, ADC reading, PWM output, valve GPIO control, endstop monitoring

This split ensures precise step timing (via PIO hardware) while keeping complex logic on a powerful Linux system.

## Communication Protocol

Host and MCU communicate over SPI at 4 MHz. The protocol uses:

- **Framing**: `[SYNC 0x4859] [SEQ] [LEN] [PAYLOAD] [CRC-16]`
- **Reliability**: Sequence numbers + ACK/retransmit
- **Clock sync**: Periodic `GET_CLOCK` to map host time ↔ MCU time
- **Step commands**: `queue_step {channel, interval, count, add}` — compact encoding of acceleration ramps
- **Valve commands**: `VALVE_SET {mask}` / `VALVE_GET` — control solenoid needle valves

See `shared/protocol_defs.h` for message definitions.

## Multi-Nozzle Valve Array

The printer uses a shared melt chamber with 4 solenoid-controlled needle valve nozzles:

- **Valve control via G-code**: `M600 V<bitmask>` selects which nozzles are open
- **Single G-code stream**: No synchronization needed — one file, one planner
- **Flow scaling**: Extruder speed is multiplied by the number of open nozzles
- **Nozzle sizes**: 0.4mm (quality), 2x 0.6mm (infill), 0.8mm (infill)

## Motion System

- **CoreXY kinematics**: Two motors (A, B) drive the shared frame. `A = X+Y`, `B = X-Y`.
- **3x Z lead screws**: Independent Z motors for auto bed leveling.
- **Single planner**: One lookahead buffer, one velocity profile generator.
- **Valve-aware moves**: Each planned move carries a valve bitmask for flow routing.

## Step Generation

The RP2040's PIO (Programmable I/O) generates step pulses:

1. Core 1 drains step queues and builds 16-bit shift register words
2. PIO shifts out step/direction bits via 74HC595 shift registers
3. This drives 7 stepper channels from just 3 GPIO pins
4. Step rates up to 230 kHz are achievable (far exceeding 3D printer needs)

## Thermal Control

- PID runs on the host at 10 Hz
- MCU reads thermistors via ADC at 100 Hz and reports to host
- Single manifold heater for the shared melt chamber
- MCU has hard thermal limits (300°C manifold, 120°C bed) — kills heaters independently of host
- Watchdog: MCU kills heaters if host goes silent for 500ms
