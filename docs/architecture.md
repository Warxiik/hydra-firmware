# Architecture

## Overview

Hydra uses a split architecture inspired by Klipper:

- **Host (Raspberry Pi CM5)**: All "smart" logic — G-code parsing, motion planning, synchronization, thermal control, UI
- **MCU (RP2040)**: Real-time I/O — step pulse generation, ADC reading, PWM output, endstop monitoring

This split ensures precise step timing (via PIO hardware) while keeping complex logic on a powerful Linux system.

## Communication Protocol

Host and MCU communicate over SPI at 4 MHz. The protocol uses:

- **Framing**: `[SYNC 0x4859] [SEQ] [LEN] [PAYLOAD] [CRC-16]`
- **Reliability**: Sequence numbers + ACK/retransmit
- **Clock sync**: Periodic `GET_CLOCK` to map host time ↔ MCU time
- **Step commands**: `queue_step {channel, interval, count, add}` — compact encoding of acceleration ramps

See `shared/protocol_defs.h` for message definitions.

## Dual-Nozzle Coordination

The HydraSlicer generates two G-code files with synchronization markers:

1. **Barrier mode**: `; SYNC BARRIER <id>` — both nozzles must reach the barrier before proceeding
2. **Task mode**: `; WAIT TASK <id>` / `; TASK BEGIN/END <id>` — dependency-based pipelining

The `SyncEngine` in the host software coordinates both streams, ensuring nozzles wait for each other when required.

## Motion System

- **CoreXY kinematics**: Two motors (A, B) drive the shared frame. `A = X+Y`, `B = X-Y`.
- **Nozzle offset**: Nozzle 1 has independent X/Y offset actuators relative to the frame.
- **Per-nozzle planning**: Each nozzle has its own lookahead buffer and velocity profile generator.
- **Combined acceleration**: The shared frame has a maximum acceleration budget; both nozzles' demands must fit within it.

## Step Generation

The RP2040's PIO (Programmable I/O) generates step pulses:

1. Core 1 drains step queues and builds 16-bit shift register words
2. PIO shifts out step/direction bits via 74HC595 shift registers
3. This drives 7 stepper channels from just 3 GPIO pins
4. Step rates up to 230 kHz are achievable (far exceeding 3D printer needs)

## Thermal Control

- PID runs on the host at 10 Hz
- MCU reads thermistors via ADC at 100 Hz and reports to host
- MCU has hard thermal limits (300°C nozzle, 120°C bed) — kills heaters independently of host
- Watchdog: MCU kills heaters if host goes silent for 500ms
