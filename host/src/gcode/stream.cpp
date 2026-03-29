#include "stream.h"
#include <spdlog/spdlog.h>

namespace hydra::gcode {

bool Stream::open(const std::string& path) {
    file_.open(path);
    if (!file_.is_open()) {
        spdlog::error("Failed to open G-code: {}", path);
        return false;
    }
    line_num_ = 0;
    at_eof_ = false;
    spdlog::info("Opened G-code stream: {}", path);
    return true;
}

void Stream::close() {
    file_.close();
    at_eof_ = true;
}

std::optional<Command> Stream::next() {
    if (at_eof_) return std::nullopt;

    std::string line;
    while (std::getline(file_, line)) {
        line_num_++;
        auto cmd = parser_.parse(line);
        if (!std::holds_alternative<NopCommand>(cmd)) {
            return cmd;
        }
    }

    at_eof_ = true;
    return std::nullopt;
}

void Stream::skip_to(int target_line) {
    if (at_eof_) return;

    std::string line;
    while (line_num_ < target_line && std::getline(file_, line)) {
        line_num_++;
    }

    spdlog::info("Stream skipped to line {}", line_num_);
}

int Stream::line_number() const {
    return line_num_;
}

bool Stream::eof() const {
    return at_eof_;
}

} // namespace hydra::gcode
