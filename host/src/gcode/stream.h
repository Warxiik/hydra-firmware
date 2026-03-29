#pragma once

#include "parser.h"
#include "command.h"
#include <fstream>
#include <string>
#include <optional>

namespace hydra::gcode {

/**
 * Single G-code file stream reader.
 * Multi-nozzle valve control is handled via M600 V<mask> in the stream.
 */
class Stream {
public:
    bool open(const std::string& path);
    void close();

    /* Read next command from stream */
    std::optional<Command> next();

    /* Current line number for error reporting */
    int line_number() const;

    /* Skip to a specific line number (for resume from checkpoint) */
    void skip_to(int target_line);

    /* Has this stream reached EOF? */
    bool eof() const;

private:
    std::ifstream file_;
    Parser parser_;
    int line_num_ = 0;
    bool at_eof_ = false;
};

} // namespace hydra::gcode
