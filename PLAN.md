# Hydra Firmware — Implementation Plan

## Phase 1: Foundation (MCU ↔ Host Communication)

**Goal:** A stepper motor moves according to a G-code file, end-to-end.

- [x] **1.1** MCU: SPI slave transport layer (frame encoding, CRC-16, sequence numbers)
- [x] **1.2** Host: SPI master transport layer (frame encoding, retransmit on NAK/timeout)
- [x] **1.3** Shared: Binary protocol definitions (`shared/protocol_defs.h`) — message IDs, VLQ encoding
- [x] **1.4** MCU: Protocol decode/dispatch on Core 0
- [x] **1.5** Host: Protocol encode + MCU proxy (`mcu_proxy.h` — high-level command interface)
- [x] **1.6** Host↔MCU clock synchronization (GET_CLOCK, linear regression time mapping)
- [x] **1.7** MCU: Stepper PIO program (step + dir via 74HC595 shift register)
- [x] **1.8** MCU: Step queue — ring buffer of `{interval, count, add}` segments per stepper
- [x] **1.9** Host: G-code parser (single stream, G0/G1/G28/M-codes)
- [x] **1.10** Host: Basic move planner (multi-axis, trapezoidal acceleration, lookahead)
- [x] **1.11** Host: Step compression (`interval/count/add` fitting)
- [ ] **1.12** Integration: Move a stepper motor via parsed G-code → planner → steps → SPI → PIO

**Tests:** Protocol round-trip, step timing accuracy (logic analyzer), parser coverage (14 passing)

---

## Phase 2: Full Motion System

**Goal:** CoreXY motion with all axes, homing, manual jog.

- [x] **2.1** MCU: Shift-register PIO program (`step_dir_sr.pio`) — 74HC595 drives 7 axes from PIO
- [x] **2.2** MCU: All 7 stepper channels operational via shift register
- [x] **2.3** Host: CoreXY kinematics (forward/inverse: `A = X+Y`, `B = X-Y`)
- [x] **2.4** Host: Multi-axis move planner with lookahead buffer (64 moves)
- [x] **2.5** Host: Cornering velocity calculation (junction deviation model)
- [x] **2.6** Host: Trapezoidal velocity profiles → step timing segments
- [x] **2.7** MCU: Endstop input monitoring + host notification
- [ ] **2.8** Host: Homing sequence (G28) — fast approach, slow probe, backoff (basic impl exists)
- [x] **2.9** MCU: TMC2209 UART via PIO (single-wire half-duplex, 4 drivers per bus)
- [x] **2.10** Host: TMC2209 configuration (current, microstepping, StealthChop/SpreadCycle)
- [ ] **2.11** Host: Sensorless homing via StallGuard (optional)

**Tests:** Kinematics transforms, planner velocity limits, homing repeatability

---

## Phase 3: Thermal Control + Basic UI

**Goal:** Temperature control, basic touchscreen UI, first real single-nozzle print.

- [x] **3.1** MCU: ADC round-robin thermistor reading (3 channels @ 100Hz)
- [x] **3.2** MCU: Heater PWM output (2x nozzle @ 10Hz, 1x bed @ 1Hz)
- [x] **3.3** MCU: Fan PWM output (2x part cooling @ 25kHz, 2x hotend fan)
- [x] **3.4** MCU: Hard thermal limits (MCU-side emergency cutoff >300°C nozzle, >120°C bed)
- [x] **3.5** Host: PID controller (configurable Kp/Ki/Kd per heater, anti-windup)
- [x] **3.6** Host: Thermal safety (runaway detection: no temp rise after N seconds at full power)
- [x] **3.7** Host: Fan control (M106/M107, layer-based ramp-up)
- [x] **3.8** MCU: Watchdog timer (500ms, kills heaters if host goes silent)
- [x] **3.9** UI: React project skeleton — App.tsx, theme, page navigation, Chromium kiosk
- [x] **3.10** UI: StatusBar component (temps, WiFi, IP)
- [x] **3.11** UI: HomePage (printer status, quick actions, TempGauges)
- [x] **3.12** UI: PrintPage (live temp chart, progress ring, override sliders)
- [x] **3.13** UI: MovePage (manual XYZ jog, home buttons)
- [x] **3.14** UI: PreparePage (preheat presets, home, load filament)
- [x] **3.15** Host: WebSocket bridge (live data from engine to React UI)
- [ ] **3.16** First single-nozzle print: calibration cube

**Tests:** PID convergence simulation, thermal runaway detection, UI component tests

---

## Phase 4: Dual-Nozzle Coordination

**Goal:** Two nozzles printing simultaneously — the core differentiator.

- [x] **4.1** Host: Dual G-code stream reader (two files, independent line cursors)
- [x] **4.2** Host: Sync engine — barrier mode (`; SYNC BARRIER <id>`)
- [x] **4.3** Host: Sync engine — task/dependency mode (`TASK BEGIN/END`, `WAIT TASK`)
- [x] **4.4** Host: Per-nozzle command queues (ring buffers, drained by engine)
- [x] **4.5** Host: Nozzle offset kinematics (frame position + offset actuator → nozzle position)
- [x] **4.6** Host: Dual planner — two planners feeding shared CoreXY + individual extruders/offsets
- [x] **4.7** Host: Frame motion decomposition (split dual-nozzle targets into frame + offset moves)
- [ ] **4.8** Host: Combined acceleration limiting (shared frame budget)
- [x] **4.9** Host: Nozzle collision validator (offset actuator range limits, exclusion zones)
- [x] **4.10** UI: Dual-nozzle PrintPage (progress per nozzle, dual temp display)
- [x] **4.11** UI: NozzleIndicator component (left=blue, right=orange)
- [ ] **4.12** Nozzle alignment calibration routine
- [ ] **4.13** Dual-nozzle test prints (stress test with various geometries)

**Tests:** Sync engine with mock streams (barriers, tasks, edge cases), collision validator, dual planner

---

## Phase 5: Polish + Production

**Goal:** Production-ready firmware image for Kickstarter fulfillment.

- [x] **5.1** Host: Web server (REST API + WebSocket for live data)
- [x] **5.2** Host: File manager (SD card + WiFi upload, list/delete/rename)
- [x] **5.3** UI: FilesPage (file browser, print preview, start print)
- [x] **5.4** UI: PrintPage overrides (mid-print speed %, flow %, fan %, Z offset)
- [x] **5.5** UI: SettingsPage (machine config, PID tune, network, about)
- [ ] **5.6** UI: NetworkPage (WiFi scan/connect, IP display, captive portal)
- [ ] **5.7** Host: Print recovery (periodic state checkpoint to eMMC, resume after power loss)
- [ ] **5.8** Host: OTA firmware update (host software + MCU .uf2 via UI/web)
- [ ] **5.9** Host: M600 filament change (pause, retract, UI prompt, resume)
- [ ] **5.10** UI: Filament change dialog + animation
- [ ] **5.11** CI: GitHub Actions (build host x86+ARM, build MCU, lint, tests)
- [ ] **5.12** Build: Yocto/Buildroot SD card image (meta-hydra layer)
- [ ] **5.13** Docs: User manual, build guide, API reference
- [ ] **5.14** QA: Extended print testing (8h+ prints, thermal cycling, WiFi stability)

**Tests:** Web API integration tests, OTA update flow, power-loss recovery, long-duration prints

---

## Current Status

**Phase:** Phase 1–5 core implementations mostly complete
**Build:** `hydra-host.exe` compiles (C++20, GCC 14.2), 15 tests passing
**Next steps:**
- 1.12: Hardware integration test with real RP2040
- 2.8: Full homing sequence (fast approach, slow probe, backoff)
- 4.8: Combined acceleration limiting (shared frame budget)
- 4.12: Nozzle alignment calibration routine
- 5.7: Print recovery (state checkpoint, resume after power loss)
