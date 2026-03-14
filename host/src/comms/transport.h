#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <mutex>

namespace hydra::comms {

/**
 * SPI master transport layer.
 * Handles frame encoding/decoding with CRC-16 CCITT.
 *
 * Frame format:
 *   [SYNC0 'H'][SYNC1 'Y'][SEQ][LEN_LO][LEN_HI][PAYLOAD...][CRC_LO][CRC_HI]
 */
class Transport {
public:
    explicit Transport(const std::string& spi_device, int speed_hz);
    ~Transport();

    bool open();
    void close();
    bool is_open() const { return fd_ >= 0; }

    /** Send a framed command to the MCU */
    bool send(const uint8_t* payload, uint16_t len);

    /** Full-duplex transfer: send a command frame and receive the response */
    std::optional<std::vector<uint8_t>> transfer(const uint8_t* payload, uint16_t len);

    /** Statistics */
    uint64_t frames_sent() const { return frames_sent_; }
    uint64_t frames_received() const { return frames_received_; }
    uint64_t crc_errors() const { return crc_errors_; }

private:
    static uint16_t crc16_ccitt(const uint8_t* data, uint16_t len);
    std::vector<uint8_t> encode_frame(const uint8_t* payload, uint16_t len);
    std::optional<std::vector<uint8_t>> decode_frame(const uint8_t* data, uint16_t len);
    bool spi_xfer(const uint8_t* tx, uint8_t* rx, uint16_t len);

    std::string device_;
    int speed_hz_;
    int fd_ = -1;
    uint8_t seq_ = 0;
    std::mutex mutex_;

    uint64_t frames_sent_ = 0;
    uint64_t frames_received_ = 0;
    uint64_t crc_errors_ = 0;
};

} // namespace hydra::comms
