#include "mcu_proxy.h"
#include <spdlog/spdlog.h>

namespace hydra::comms {

McuProxy::McuProxy(const Config& config)
    : transport_(std::make_unique<Transport>(config.spi_device, config.spi_speed_hz)) {}

McuProxy::~McuProxy() {
    disconnect();
}

bool McuProxy::connect() {
    if (!transport_->open()) {
        spdlog::error("Failed to open SPI transport");
        return false;
    }

    protocol_ = std::make_unique<Protocol>(*transport_);
    clock_sync_ = std::make_unique<ClockSync>(*protocol_);

    /* Initial handshake: send NOP and expect ACK */
    if (!protocol_->send_nop()) {
        spdlog::warn("MCU did not respond to NOP (may be offline)");
        /* Continue anyway — MCU might boot later */
    }

    /* Run initial clock sync samples */
    for (int i = 0; i < 4; i++) {
        clock_sync_->sync_once();
    }

    connected_ = true;
    spdlog::info("MCU proxy connected");
    return true;
}

void McuProxy::disconnect() {
    connected_ = false;
    protocol_.reset();
    clock_sync_.reset();
    transport_->close();
}

bool McuProxy::sync_clock() {
    if (!protocol_) return false;
    return clock_sync_->sync_once();
}

bool McuProxy::is_clock_synced() const {
    return clock_sync_ && clock_sync_->is_calibrated();
}

uint32_t McuProxy::estimated_mcu_clock() const {
    if (!clock_sync_) return 0;
    return clock_sync_->estimated_mcu_clock();
}

bool McuProxy::queue_step(uint8_t channel, uint32_t interval, uint16_t count, int16_t add) {
    if (!protocol_) return false;
    return protocol_->queue_step(channel, interval, count, add);
}

bool McuProxy::set_step_dir(uint8_t channel, uint8_t dir) {
    if (!protocol_) return false;
    return protocol_->set_step_dir(channel, dir);
}

bool McuProxy::reset_step_clock(uint8_t channel, uint32_t clock) {
    if (!protocol_) return false;
    return protocol_->reset_step_clock(channel, clock);
}

bool McuProxy::set_heater_pwm(uint8_t channel, uint16_t duty) {
    if (!protocol_) return false;
    return protocol_->set_pwm(channel, duty);
}

bool McuProxy::set_fan_pwm(uint8_t channel, uint16_t duty) {
    if (!protocol_) return false;
    return protocol_->set_pwm(channel, duty);
}

bool McuProxy::poll_status() {
    if (!protocol_) return false;
    auto report = protocol_->get_status();
    if (!report) return false;

    status_.mcu_clock = report->mcu_clock;
    std::copy(std::begin(report->adc_raw), std::end(report->adc_raw), std::begin(status_.adc_raw));
    status_.endstop_state = report->endstop_state;
    std::copy(std::begin(report->queue_depth), std::end(report->queue_depth), std::begin(status_.queue_depth));
    status_.valve_state = report->valve_state;
    status_.flags = report->flags;
    return true;
}

bool McuProxy::emergency_stop() {
    if (!protocol_) return false;
    spdlog::error("Sending EMERGENCY STOP to MCU");
    return protocol_->emergency_stop();
}

bool McuProxy::heartbeat() {
    if (!protocol_) return false;
    return protocol_->send_nop();
}

bool McuProxy::endstop_home(uint8_t channel, uint8_t endstop_bit) {
    if (!protocol_) return false;
    return protocol_->endstop_home(channel, endstop_bit);
}

std::optional<McuProxy::EndstopQuery> McuProxy::endstop_query() {
    if (!protocol_) return std::nullopt;
    auto result = protocol_->endstop_query();
    if (!result) return std::nullopt;
    return EndstopQuery{result->endstop_state, result->trigger_flags};
}

bool McuProxy::tmc_write(uint8_t driver, uint8_t reg, uint32_t value) {
    if (!protocol_) return false;
    return protocol_->tmc_write(driver, reg, value);
}

std::optional<uint32_t> McuProxy::tmc_read(uint8_t driver, uint8_t reg) {
    if (!protocol_) return std::nullopt;
    return protocol_->tmc_read(driver, reg);
}

bool McuProxy::send_valve_set(uint8_t mask) {
    if (!protocol_) return false;
    return protocol_->valve_set(mask);
}

std::optional<uint8_t> McuProxy::send_valve_get() {
    if (!protocol_) return std::nullopt;
    return protocol_->valve_get();
}

bool McuProxy::queue_has_space(uint8_t channel, uint8_t min_free) const {
    if (channel >= STEPPER_COUNT) return false;
    return status_.queue_depth[channel] < (255 - min_free);
}

} // namespace hydra::comms
