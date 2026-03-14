#pragma once

#include "parser.h"
#include "command.h"
#include <fstream>
#include <string>
#include <optional>

namespace hydra::gcode {

/**
 * Manages dual G-code file streams (one per nozzle).
 * Files are named: <base>_nozzle0.gcode, <base>_nozzle1.gcode
 */
class Stream {
public:
    bool open(const std::string& base_path, int nozzle_count = 2);
    void close();

    /* Read next command from a nozzle's stream */
    std::optional<Command> next(int nozzle_id);

    /* Current line number for error reporting */
    int line_number(int nozzle_id) const;

    /* Skip to a specific line number (for resume from checkpoint) */
    void skip_to(int nozzle_id, int target_line);

    /* Has this stream reached EOF? */
    bool eof(int nozzle_id) const;

    /* Are all streams at EOF? */
    bool all_done() const;

private:
    struct NozzleStream {
        std::ifstream file;
        Parser parser;
        int line_num = 0;
        bool at_eof = false;
    };

    NozzleStream streams_[2];
    int nozzle_count_ = 0;
};

} // namespace hydra::gcode
