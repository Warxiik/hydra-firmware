#include "parser.h"
#include <charconv>
#include <algorithm>
#include <cstring>

namespace hydra::gcode {

static std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r'))
        s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
        s.remove_suffix(1);
    return s;
}

std::optional<double> Parser::extract_param(std::string_view line, char param) {
    for (size_t i = 0; i < line.size(); i++) {
        if ((line[i] == param || line[i] == (param + 32)) && i + 1 < line.size()) {
            double val;
            auto [ptr, ec] = std::from_chars(line.data() + i + 1, line.data() + line.size(), val);
            if (ec == std::errc{}) return val;
        }
    }
    return std::nullopt;
}

std::optional<int> Parser::extract_int_param(std::string_view line, char param) {
    auto val = extract_param(line, param);
    if (val) return static_cast<int>(*val);
    return std::nullopt;
}

Command Parser::parse(std::string_view line) {
    line = trim(line);
    if (line.empty()) return NopCommand{};

    /* Skip comment-only lines */
    if (line.front() == ';') return NopCommand{};

    /* Strip inline comments */
    auto comment_pos = line.find(';');
    std::string_view code_part = (comment_pos != std::string_view::npos)
        ? trim(line.substr(0, comment_pos))
        : line;

    if (code_part.empty()) return NopCommand{};

    char letter = code_part.front();
    int code_num = 0;
    std::from_chars(code_part.data() + 1, code_part.data() + code_part.size(), code_num);

    if (letter == 'G' || letter == 'g') return parse_g_command(code_part, code_num);
    if (letter == 'M' || letter == 'm') return parse_m_command(code_part, code_num);

    return NopCommand{};
}

Command Parser::parse_g_command(std::string_view line, int code) {
    switch (code) {
    case 0:
    case 1: {
        MoveCommand cmd;
        cmd.rapid = (code == 0);
        cmd.x = extract_param(line, 'X');
        cmd.y = extract_param(line, 'Y');
        cmd.z = extract_param(line, 'Z');
        cmd.e = extract_param(line, 'E');
        cmd.f = extract_param(line, 'F');
        return cmd;
    }
    case 28: {
        HomeCommand cmd;
        /* G28 with no args homes all; G28 X homes just X */
        bool has_any = line.find('X') != std::string_view::npos ||
                       line.find('Y') != std::string_view::npos ||
                       line.find('Z') != std::string_view::npos;
        if (has_any) {
            cmd.x = line.find('X') != std::string_view::npos;
            cmd.y = line.find('Y') != std::string_view::npos;
            cmd.z = line.find('Z') != std::string_view::npos;
        }
        return cmd;
    }
    case 90: /* Absolute positioning — handled by stream state */
    case 91: /* Relative positioning */
    case 92: {
        SetPosition cmd;
        cmd.x = extract_param(line, 'X');
        cmd.y = extract_param(line, 'Y');
        cmd.z = extract_param(line, 'Z');
        cmd.e = extract_param(line, 'E');
        return cmd;
    }
    default:
        return NopCommand{};
    }
}

Command Parser::parse_m_command(std::string_view line, int code) {
    switch (code) {
    case 104: { /* Set nozzle temp (no wait) */
        TempCommand cmd;
        cmd.target = TempCommand::Nozzle;
        cmd.temp_c = extract_param(line, 'S').value_or(0);
        cmd.nozzle_id = extract_int_param(line, 'T').value_or(0);
        cmd.wait = false;
        return cmd;
    }
    case 109: { /* Set nozzle temp (wait) */
        TempCommand cmd;
        cmd.target = TempCommand::Nozzle;
        cmd.temp_c = extract_param(line, 'S').value_or(0);
        cmd.nozzle_id = extract_int_param(line, 'T').value_or(0);
        cmd.wait = true;
        return cmd;
    }
    case 140: {
        TempCommand cmd;
        cmd.target = TempCommand::Bed;
        cmd.temp_c = extract_param(line, 'S').value_or(0);
        cmd.wait = false;
        return cmd;
    }
    case 190: {
        TempCommand cmd;
        cmd.target = TempCommand::Bed;
        cmd.temp_c = extract_param(line, 'S').value_or(0);
        cmd.wait = true;
        return cmd;
    }
    case 106: {
        FanCommand cmd;
        cmd.speed = extract_int_param(line, 'S').value_or(255);
        cmd.fan_id = extract_int_param(line, 'P').value_or(0);
        return cmd;
    }
    case 107: {
        FanCommand cmd;
        cmd.speed = -1; /* Off */
        cmd.fan_id = extract_int_param(line, 'P').value_or(0);
        return cmd;
    }
    case 204: {
        AccelCommand cmd;
        cmd.print_accel = extract_param(line, 'S');
        cmd.travel_accel = extract_param(line, 'T');
        return cmd;
    }
    case 205: {
        AccelCommand cmd;
        cmd.jerk_x = extract_param(line, 'X');
        cmd.jerk_y = extract_param(line, 'Y');
        return cmd;
    }
    case 600: {
        /* M600 V<mask> — valve control. No V param = filament change. */
        auto valve_mask = extract_int_param(line, 'V');
        if (valve_mask) {
            ValveSet cmd;
            cmd.mask = static_cast<uint8_t>(*valve_mask & 0x0F);
            return cmd;
        }
        return FilamentChange{};
    }
    case 82: /* Absolute extrusion */
    case 83: /* Relative extrusion */
    case 84: /* Disable motors */
        return NopCommand{}; /* Handled by stream state or engine */
    default:
        return NopCommand{};
    }
}

} // namespace hydra::gcode
