#include "protocol.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace hydra::comms {

Protocol::Protocol(Transport& transport) : transport_(transport) {}

std::optional<std::vector<uint8_t>> Protocol::command(const uint8_t* cmd, uint16_t len) {
    auto rsp = transport_.transfer(cmd, len);
    if (!rsp || rsp->empty()) {
        return std::nullopt;
    }
    /* Check for error response */
    if ((*rsp)[0] == RSP_ERROR && rsp->size() >= 2) {
        spdlog::warn("MCU error response: code={:#x}", (*rsp)[1]);
        return std::nullopt;
    }
    return rsp;
}

bool Protocol::send_nop() {
    uint8_t cmd = CMD_NOP;
    auto rsp = command(&cmd, 1);
    return rsp && !rsp->empty() && (*rsp)[0] == RSP_ACK;
}

std::optional<uint32_t> Protocol::get_clock() {
    uint8_t cmd = CMD_GET_CLOCK;
    auto rsp = command(&cmd, 1);
    if (!rsp || rsp->size() < 5 || (*rsp)[0] != RSP_CLOCK)
        return std::nullopt;

    uint32_t clock = (*rsp)[1]
                   | (static_cast<uint32_t>((*rsp)[2]) << 8)
                   | (static_cast<uint32_t>((*rsp)[3]) << 16)
                   | (static_cast<uint32_t>((*rsp)[4]) << 24);
    return clock;
}

bool Protocol::emergency_stop() {
    uint8_t cmd = CMD_EMERGENCY_STOP;
    auto rsp = command(&cmd, 1);
    return rsp && !rsp->empty() && (*rsp)[0] == RSP_ACK;
}

bool Protocol::queue_step(uint8_t channel, uint32_t interval, uint16_t count, int16_t add) {
    uint8_t buf[1 + sizeof(hydra_queue_step_t)];
    buf[0] = CMD_QUEUE_STEP;

    hydra_queue_step_t step;
    step.oid = channel;
    step.interval = interval;
    step.count = count;
    step.add = add;
    std::memcpy(&buf[1], &step, sizeof(step));

    auto rsp = command(buf, sizeof(buf));
    return rsp && !rsp->empty() && (*rsp)[0] == RSP_ACK;
}

bool Protocol::set_step_dir(uint8_t channel, uint8_t dir) {
    uint8_t buf[3] = {CMD_SET_NEXT_STEP_DIR, channel, dir};
    transport_.send(buf, 3); /* Fire-and-forget */
    return true;
}

bool Protocol::reset_step_clock(uint8_t channel, uint32_t clock) {
    uint8_t buf[6];
    buf[0] = CMD_RESET_STEP_CLOCK;
    buf[1] = channel;
    buf[2] = clock & 0xFF;
    buf[3] = (clock >> 8) & 0xFF;
    buf[4] = (clock >> 16) & 0xFF;
    buf[5] = (clock >> 24) & 0xFF;
    transport_.send(buf, 6);
    return true;
}

std::optional<int32_t> Protocol::get_stepper_position(uint8_t channel) {
    uint8_t buf[2] = {CMD_STEPPER_GET_POS, channel};
    auto rsp = command(buf, 2);
    if (!rsp || rsp->size() < 6 || (*rsp)[0] != RSP_STEPPER_POS)
        return std::nullopt;

    int32_t pos = static_cast<int32_t>(
        (*rsp)[2]
        | (static_cast<uint32_t>((*rsp)[3]) << 8)
        | (static_cast<uint32_t>((*rsp)[4]) << 16)
        | (static_cast<uint32_t>((*rsp)[5]) << 24)
    );
    return pos;
}

bool Protocol::set_pwm(uint8_t channel, uint16_t duty) {
    uint8_t buf[4];
    buf[0] = CMD_SET_PWM;
    buf[1] = channel;
    buf[2] = duty & 0xFF;
    buf[3] = (duty >> 8) & 0xFF;
    transport_.send(buf, 4);
    return true;
}

bool Protocol::endstop_home(uint8_t channel, uint8_t endstop_bit) {
    uint8_t buf[3] = {CMD_ENDSTOP_HOME, channel, endstop_bit};
    auto rsp = command(buf, 3);
    return rsp && !rsp->empty() && (*rsp)[0] == RSP_ACK;
}

std::optional<Protocol::EndstopQueryResult> Protocol::endstop_query() {
    uint8_t cmd = CMD_ENDSTOP_QUERY;
    auto rsp = command(&cmd, 1);
    if (!rsp || rsp->size() < 3 || (*rsp)[0] != RSP_ENDSTOP_STATE)
        return std::nullopt;
    return EndstopQueryResult{(*rsp)[1], (*rsp)[2]};
}

bool Protocol::tmc_write(uint8_t driver, uint8_t reg, uint32_t value) {
    uint8_t buf[7];
    buf[0] = CMD_TMC_WRITE;
    buf[1] = driver;
    buf[2] = reg;
    buf[3] = value & 0xFF;
    buf[4] = (value >> 8) & 0xFF;
    buf[5] = (value >> 16) & 0xFF;
    buf[6] = (value >> 24) & 0xFF;
    auto rsp = command(buf, 7);
    return rsp && !rsp->empty() && (*rsp)[0] == RSP_ACK;
}

std::optional<uint32_t> Protocol::tmc_read(uint8_t driver, uint8_t reg) {
    uint8_t buf[3] = {CMD_TMC_READ, driver, reg};
    auto rsp = command(buf, 3);
    if (!rsp || rsp->size() < 5 || (*rsp)[0] != RSP_TMC_READ)
        return std::nullopt;

    uint32_t value = (*rsp)[1]
                   | (static_cast<uint32_t>((*rsp)[2]) << 8)
                   | (static_cast<uint32_t>((*rsp)[3]) << 16)
                   | (static_cast<uint32_t>((*rsp)[4]) << 24);
    return value;
}

std::optional<Protocol::StatusReport> Protocol::get_status() {
    uint8_t cmd = CMD_GET_STATUS;
    auto rsp = command(&cmd, 1);
    if (!rsp || rsp->size() < 1 + sizeof(hydra_status_report_t) || (*rsp)[0] != RSP_STATUS)
        return std::nullopt;

    StatusReport report;
    const auto* raw = reinterpret_cast<const hydra_status_report_t*>(rsp->data() + 1);
    report.mcu_clock = raw->mcu_clock;
    std::memcpy(report.adc_raw, raw->adc_raw, sizeof(report.adc_raw));
    report.endstop_state = raw->endstop_state;
    std::memcpy(report.queue_depth, raw->queue_depth, sizeof(report.queue_depth));
    report.flags = raw->flags;
    return report;
}

} // namespace hydra::comms
