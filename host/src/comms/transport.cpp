#include "transport.h"
#include "protocol_defs.h"
#include <spdlog/spdlog.h>
#include <cstring>

#ifdef __linux__
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

namespace hydra::comms {

Transport::Transport(const std::string& spi_device, int speed_hz)
    : device_(spi_device), speed_hz_(speed_hz) {}

Transport::~Transport() {
    close();
}

uint16_t Transport::crc16_ccitt(const uint8_t* data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

bool Transport::open() {
#ifdef __linux__
    fd_ = ::open(device_.c_str(), O_RDWR);
    if (fd_ < 0) {
        spdlog::error("Failed to open SPI device: {}", device_);
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = static_cast<uint32_t>(speed_hz_);

    ioctl(fd_, SPI_IOC_WR_MODE, &mode);
    ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    spdlog::info("SPI transport opened: {} @ {}Hz", device_, speed_hz_);
    return true;
#else
    spdlog::warn("SPI transport: no Linux SPI support on this platform (mock mode)");
    fd_ = 9999; /* Fake fd for non-Linux development */
    return true;
#endif
}

void Transport::close() {
#ifdef __linux__
    if (fd_ >= 0) {
        ::close(fd_);
    }
#endif
    fd_ = -1;
}

std::vector<uint8_t> Transport::encode_frame(const uint8_t* payload, uint16_t len) {
    std::vector<uint8_t> frame;
    frame.reserve(HYDRA_FRAME_OVERHEAD + len);

    /* Sync bytes */
    frame.push_back(HYDRA_SYNC_BYTE_0);
    frame.push_back(HYDRA_SYNC_BYTE_1);

    /* Sequence number */
    frame.push_back(seq_++);

    /* Length (little-endian) */
    frame.push_back(len & 0xFF);
    frame.push_back((len >> 8) & 0xFF);

    /* Payload */
    frame.insert(frame.end(), payload, payload + len);

    /* CRC-16 over seq + len + payload (bytes 2..end) */
    uint16_t crc = crc16_ccitt(frame.data() + 2, 3 + len);
    frame.push_back(crc & 0xFF);
    frame.push_back((crc >> 8) & 0xFF);

    return frame;
}

std::optional<std::vector<uint8_t>> Transport::decode_frame(const uint8_t* data, uint16_t len) {
    /* Scan for sync bytes */
    for (uint16_t i = 0; i + HYDRA_FRAME_OVERHEAD <= len; i++) {
        if (data[i] != HYDRA_SYNC_BYTE_0 || data[i + 1] != HYDRA_SYNC_BYTE_1)
            continue;

        /* Parse length */
        uint16_t payload_len = data[i + 3] | (static_cast<uint16_t>(data[i + 4]) << 8);
        if (payload_len > HYDRA_MAX_PAYLOAD) continue;

        uint16_t frame_len = HYDRA_FRAME_OVERHEAD + payload_len;
        if (i + frame_len > len) continue;

        /* Validate CRC */
        uint16_t crc_data_len = 3 + payload_len; /* seq + len(2) + payload */
        uint16_t computed = crc16_ccitt(&data[i + 2], crc_data_len);
        uint16_t received = data[i + 5 + payload_len]
                          | (static_cast<uint16_t>(data[i + 6 + payload_len]) << 8);

        if (computed != received) {
            crc_errors_++;
            continue;
        }

        /* Extract payload */
        frames_received_++;
        return std::vector<uint8_t>(data + i + 5, data + i + 5 + payload_len);
    }
    return std::nullopt;
}

bool Transport::spi_xfer(const uint8_t* tx, uint8_t* rx, uint16_t len) {
#ifdef __linux__
    struct spi_ioc_transfer tr{};
    tr.tx_buf = reinterpret_cast<unsigned long>(tx);
    tr.rx_buf = reinterpret_cast<unsigned long>(rx);
    tr.len = len;
    tr.speed_hz = static_cast<uint32_t>(speed_hz_);
    tr.bits_per_word = 8;

    int ret = ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
    return ret >= 0;
#else
    /* Mock: zero-fill rx */
    std::memset(rx, 0, len);
    return true;
#endif
}

bool Transport::send(const uint8_t* payload, uint16_t len) {
    if (!is_open() || len > HYDRA_MAX_PAYLOAD) return false;

    std::lock_guard lock(mutex_);
    auto frame = encode_frame(payload, len);

    std::vector<uint8_t> dummy(frame.size(), 0);
    bool ok = spi_xfer(frame.data(), dummy.data(), static_cast<uint16_t>(frame.size()));
    if (ok) frames_sent_++;
    return ok;
}

std::optional<std::vector<uint8_t>> Transport::transfer(const uint8_t* payload, uint16_t len) {
    if (!is_open() || len > HYDRA_MAX_PAYLOAD) return std::nullopt;

    std::lock_guard lock(mutex_);
    auto frame = encode_frame(payload, len);

    /* We send our frame and simultaneously receive the MCU's response.
     * The MCU may need a bit more space to respond, so pad the transfer. */
    uint16_t xfer_len = static_cast<uint16_t>(frame.size() + HYDRA_MAX_FRAME);
    frame.resize(xfer_len, 0x00); /* Pad with zeros */

    std::vector<uint8_t> rx(xfer_len, 0);
    bool ok = spi_xfer(frame.data(), rx.data(), xfer_len);
    if (!ok) return std::nullopt;

    frames_sent_++;
    return decode_frame(rx.data(), xfer_len);
}

} // namespace hydra::comms
