# Host ↔ MCU Protocol

## Transport Layer

SPI at 4 MHz. CM5 is master, RP2040 is slave.

### Frame Format

```
[SYNC_0: 0x48] [SYNC_1: 0x59] [SEQ: u8] [LEN_LO: u8] [LEN_HI: u8] [PAYLOAD: N bytes] [CRC_LO: u8] [CRC_HI: u8]
```

- Max payload: 56 bytes
- CRC-16 CCITT over `SEQ + LEN + PAYLOAD`
- SEQ: low nibble is 4-bit sequence counter, high nibble = 0x10

### Reliability

- Host→MCU: acknowledged (MCU sends RSP_ACK)
- MCU→Host: fire-and-forget (status reports)
- Host retransmits after 10ms timeout, up to 3 retries

## Commands

### Host → MCU

| ID | Name | Payload | Description |
|----|------|---------|-------------|
| 0x00 | NOP | - | Heartbeat |
| 0x01 | GET_CLOCK | - | Request MCU clock |
| 0x02 | GET_STATUS | - | Request full status |
| 0x03 | EMERGENCY_STOP | - | Halt everything |
| 0x11 | QUEUE_STEP | `{oid:u8, interval:u32, count:u16, add:i16}` | Queue step segment |
| 0x12 | SET_NEXT_STEP_DIR | `{oid:u8, dir:u8}` | Set direction |
| 0x13 | RESET_STEP_CLOCK | `{oid:u8, clock:u32}` | Reset timing reference |
| 0x20 | SET_PWM | `{channel:u8, duty:u16}` | Set PWM output |
| 0x30 | TMC_WRITE | `{bus:u8, addr:u8, reg:u8, data:u32}` | Write TMC2209 register |

### MCU → Host

| ID | Name | Payload | Description |
|----|------|---------|-------------|
| 0x81 | CLOCK | `{clock:u32}` | MCU clock value |
| 0x82 | STATUS | `hydra_status_report_t` | Periodic status (100Hz) |
| 0x83 | ACK | - | Command accepted |
| 0x84 | STEPPER_POS | `{oid:u8, pos:i32}` | Step count |
| 0xFF | ERROR | `{code:u8}` | Error report |

## Step Timing Model

```
queue_step { interval, count, add }

step[0] at: reference_clock + interval
step[i] at: step[i-1] + interval + add * i

add = 0  → constant velocity
add < 0  → acceleration (intervals shrink)
add > 0  → deceleration (intervals grow)
```

## Clock Synchronization

1. Host sends GET_CLOCK periodically (~1Hz)
2. MCU responds with its `time_us_32()` value
3. Host maintains linear regression: `mcu_clock = a * host_time + b`
4. Step commands use MCU clock values for timing
5. Typical accuracy: < 1μs drift over seconds
