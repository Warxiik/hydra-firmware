#include "protocol.h"
#include "transport.h"
#include "stepper.h"
#include "tmc_uart.h"
#include "adc.h"
#include "gpio.h"
#include "valve_control.h"
#include "watchdog.h"
#include "protocol_defs.h"
#include "config.h"
#include "pico/time.h"

static uint8_t cmd_buf[HYDRA_MAX_PAYLOAD];
static uint8_t rsp_buf[HYDRA_MAX_PAYLOAD];
static absolute_time_t next_status_time;

void protocol_init(void) {
    next_status_time = get_absolute_time();
}

static void handle_command(const uint8_t *data, uint16_t len) {
    if (len == 0) return;

    uint8_t cmd_id = data[0];

    switch (cmd_id) {
    case CMD_NOP:
        /* Heartbeat — just ACK and feed watchdog */
        hydra_watchdog_feed();
        rsp_buf[0] = RSP_ACK;
        transport_send(rsp_buf, 1);
        break;

    case CMD_GET_CLOCK: {
        rsp_buf[0] = RSP_CLOCK;
        uint32_t clock = time_us_32();
        rsp_buf[1] = clock & 0xFF;
        rsp_buf[2] = (clock >> 8) & 0xFF;
        rsp_buf[3] = (clock >> 16) & 0xFF;
        rsp_buf[4] = (clock >> 24) & 0xFF;
        transport_send(rsp_buf, 5);
        break;
    }

    case CMD_EMERGENCY_STOP:
        stepper_emergency_stop();
        hydra_gpio_kill_heaters();
        valve_control_close_all();
        rsp_buf[0] = RSP_ACK;
        transport_send(rsp_buf, 1);
        break;

    case CMD_QUEUE_STEP: {
        if (len < sizeof(hydra_queue_step_t) + 1) break;
        const hydra_queue_step_t *step = (const hydra_queue_step_t *)&data[1];
        bool ok = stepper_queue(step->oid, step);
        rsp_buf[0] = ok ? RSP_ACK : RSP_ERROR;
        if (!ok) rsp_buf[1] = ERR_QUEUE_OVERFLOW;
        transport_send(rsp_buf, ok ? 1 : 2);
        break;
    }

    case CMD_SET_NEXT_STEP_DIR:
        if (len >= 3) {
            stepper_set_dir(data[1], data[2]);
        }
        break;

    case CMD_SET_PWM:
        if (len >= 4) {
            uint8_t channel = data[1];
            uint16_t duty = data[2] | (data[3] << 8);
            hydra_gpio_set_pwm(channel, duty);
        }
        break;

    case CMD_STEPPER_GET_POS: {
        if (len >= 2) {
            int32_t pos = stepper_get_position(data[1]);
            rsp_buf[0] = RSP_STEPPER_POS;
            rsp_buf[1] = data[1]; /* channel */
            rsp_buf[2] = pos & 0xFF;
            rsp_buf[3] = (pos >> 8) & 0xFF;
            rsp_buf[4] = (pos >> 16) & 0xFF;
            rsp_buf[5] = (pos >> 24) & 0xFF;
            transport_send(rsp_buf, 6);
        }
        break;
    }

    case CMD_ENDSTOP_HOME:
        if (len >= 3) {
            stepper_set_homing(data[1], data[2]);
            rsp_buf[0] = RSP_ACK;
            transport_send(rsp_buf, 1);
        }
        break;

    case CMD_ENDSTOP_QUERY: {
        uint8_t endstops = hydra_gpio_read_endstops();
        uint8_t triggers = 0;
        for (int i = 0; i < STEPPER_COUNT; i++) {
            if (stepper_homing_triggered(i)) {
                triggers |= (1 << i);
            }
        }
        rsp_buf[0] = RSP_ENDSTOP_STATE;
        rsp_buf[1] = endstops;
        rsp_buf[2] = triggers;
        transport_send(rsp_buf, 3);
        break;
    }

    case CMD_RESET_STEP_CLOCK:
        if (len >= 6) {
            uint32_t clock = data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24);
            stepper_reset_clock(data[1], clock);
        }
        break;

    case CMD_TMC_WRITE:
        if (len >= 7) {
            uint32_t value = data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24);
            uint8_t driver = data[1];
            uint8_t bus = (driver <= 3) ? 0 : 1;
            uint8_t addr = (driver <= 3) ? driver : (driver - 4);
            bool ok = tmc_uart_write_reg(bus, addr, data[2], value);
            rsp_buf[0] = ok ? RSP_ACK : RSP_ERROR;
            if (!ok) rsp_buf[1] = ERR_TMC_COMM;
            transport_send(rsp_buf, ok ? 1 : 2);
        }
        break;

    case CMD_TMC_READ:
        if (len >= 3) {
            uint8_t driver = data[1];
            uint8_t bus = (driver <= 3) ? 0 : 1;
            uint8_t addr = (driver <= 3) ? driver : (driver - 4);
            uint32_t value = 0;
            bool ok = tmc_uart_read_reg(bus, addr, data[2], &value);
            if (ok) {
                rsp_buf[0] = RSP_TMC_READ;
                rsp_buf[1] = value & 0xFF;
                rsp_buf[2] = (value >> 8) & 0xFF;
                rsp_buf[3] = (value >> 16) & 0xFF;
                rsp_buf[4] = (value >> 24) & 0xFF;
                transport_send(rsp_buf, 5);
            } else {
                rsp_buf[0] = RSP_ERROR;
                rsp_buf[1] = ERR_TMC_COMM;
                transport_send(rsp_buf, 2);
            }
        }
        break;

    case CMD_VALVE_SET:
        if (len >= 2) {
            valve_control_set(data[1]);
            rsp_buf[0] = RSP_ACK;
            transport_send(rsp_buf, 1);
        }
        break;

    case CMD_VALVE_GET: {
        rsp_buf[0] = RSP_VALVE_STATE;
        rsp_buf[1] = valve_control_get();
        transport_send(rsp_buf, 2);
        break;
    }

    default:
        rsp_buf[0] = RSP_ERROR;
        rsp_buf[1] = ERR_BAD_CMD;
        transport_send(rsp_buf, 2);
        break;
    }
}

void protocol_process(void) {
    uint16_t len = transport_receive(cmd_buf, sizeof(cmd_buf));
    if (len > 0) {
        handle_command(cmd_buf, len);
    }
}

void protocol_send_status(void) {
    absolute_time_t now = get_absolute_time();
    if (absolute_time_diff_us(next_status_time, now) < 0) return;

    next_status_time = delayed_by_us(now, 1000000 / STATUS_REPORT_HZ);

    hydra_status_report_t report;
    report.mcu_clock = time_us_32();
    hydra_adc_read_all(report.adc_raw);
    report.endstop_state = hydra_gpio_read_endstops();
    for (int i = 0; i < STEPPER_COUNT; i++) {
        report.queue_depth[i] = stepper_queue_depth(i);
    }
    report.valve_state = valve_control_get();
    report.flags = 0;
    /* TODO: Set flags based on safety state */

    rsp_buf[0] = RSP_STATUS;
    __builtin_memcpy(&rsp_buf[1], &report, sizeof(report));
    transport_send(rsp_buf, 1 + sizeof(report));
}
