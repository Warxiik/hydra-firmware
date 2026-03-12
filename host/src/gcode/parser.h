#pragma once

#include "command.h"
#include <string_view>

namespace hydra::gcode {

/**
 * Stateless G-code line parser.
 * Parses a single line into a Command variant.
 * Also extracts Hydra sync markers from comments.
 */
class Parser {
public:
    Command parse(std::string_view line);

private:
    Command parse_g_command(std::string_view line, int code);
    Command parse_m_command(std::string_view line, int code);
    Command parse_comment(std::string_view comment);

    static std::optional<double> extract_param(std::string_view line, char param);
    static std::optional<int> extract_int_param(std::string_view line, char param);
};

} // namespace hydra::gcode
