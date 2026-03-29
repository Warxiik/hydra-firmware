#pragma once

#include "command.h"
#include <string_view>

namespace hydra::gcode {

/**
 * Stateless G-code line parser.
 * Parses a single line into a Command variant.
 */
class Parser {
public:
    Command parse(std::string_view line);

private:
    Command parse_g_command(std::string_view line, int code);
    Command parse_m_command(std::string_view line, int code);

    static std::optional<double> extract_param(std::string_view line, char param);
    static std::optional<int> extract_int_param(std::string_view line, char param);
};

} // namespace hydra::gcode
