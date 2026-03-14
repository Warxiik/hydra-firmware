#include "stream.h"
#include <spdlog/spdlog.h>

namespace hydra::gcode {

bool Stream::open(const std::string& base_path, int nozzle_count) {
    nozzle_count_ = nozzle_count;

    for (int i = 0; i < nozzle_count; i++) {
        std::string path = base_path + "_nozzle" + std::to_string(i) + ".gcode";
        streams_[i].file.open(path);
        if (!streams_[i].file.is_open()) {
            spdlog::error("Failed to open G-code: {}", path);
            close();
            return false;
        }
        streams_[i].line_num = 0;
        streams_[i].at_eof = false;
        spdlog::info("Opened G-code stream [nozzle {}]: {}", i, path);
    }
    return true;
}

void Stream::close() {
    for (int i = 0; i < 2; i++) {
        streams_[i].file.close();
        streams_[i].at_eof = true;
    }
}

std::optional<Command> Stream::next(int nozzle_id) {
    if (nozzle_id < 0 || nozzle_id >= nozzle_count_) return std::nullopt;

    auto& s = streams_[nozzle_id];
    if (s.at_eof) return std::nullopt;

    std::string line;
    while (std::getline(s.file, line)) {
        s.line_num++;
        auto cmd = s.parser.parse(line);
        if (!std::holds_alternative<NopCommand>(cmd)) {
            return cmd;
        }
    }

    s.at_eof = true;
    return std::nullopt;
}

void Stream::skip_to(int nozzle_id, int target_line) {
    if (nozzle_id < 0 || nozzle_id >= nozzle_count_) return;

    auto& s = streams_[nozzle_id];
    if (s.at_eof) return;

    /* Fast-forward by reading and discarding lines */
    std::string line;
    while (s.line_num < target_line && std::getline(s.file, line)) {
        s.line_num++;
    }

    spdlog::info("Stream [nozzle {}] skipped to line {}", nozzle_id, s.line_num);
}

int Stream::line_number(int nozzle_id) const {
    if (nozzle_id < 0 || nozzle_id >= nozzle_count_) return 0;
    return streams_[nozzle_id].line_num;
}

bool Stream::eof(int nozzle_id) const {
    if (nozzle_id < 0 || nozzle_id >= nozzle_count_) return true;
    return streams_[nozzle_id].at_eof;
}

bool Stream::all_done() const {
    for (int i = 0; i < nozzle_count_; i++) {
        if (!streams_[i].at_eof) return false;
    }
    return true;
}

} // namespace hydra::gcode
