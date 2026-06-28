#pragma once

#include <cstdint>

namespace dante::term {

// Cor de uma célula: padrão do terminal, indexada (0-255) ou RGB truecolor.
struct Color {
    enum class Kind : std::uint8_t { Default, Indexed, Rgb };

    Kind kind = Kind::Default;
    std::uint8_t index = 0;             // p/ Indexed (0-15 básicas, 16-255 estendidas)
    std::uint8_t r = 0, g = 0, b = 0;   // p/ Rgb

    static Color defaultColor() { return {}; }
    static Color indexed(std::uint8_t i) { return {Kind::Indexed, i, 0, 0, 0}; }
    static Color rgb(std::uint8_t rr, std::uint8_t gg, std::uint8_t bb) {
        return {Kind::Rgb, 0, rr, gg, bb};
    }

    bool operator==(const Color&) const = default;
};

} // namespace dante::term
