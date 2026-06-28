#pragma once

#include "dante/terminal/Color.hpp"

namespace dante::term {

// Uma célula da grade do terminal: glifo (codepoint) + atributos.
struct Cell {
    char32_t ch = U' ';
    Color fg{};
    Color bg{};
    bool bold = false;

    bool operator==(const Cell&) const = default;
};

} // namespace dante::term
