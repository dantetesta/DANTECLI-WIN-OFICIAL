#pragma once

#include <expected>
#include <string>
#include <utility>

namespace dante {

// Error carries a human-readable message. F0 needs nothing richer; add a code
// enum here when callers must branch on failure kind.
struct Error {
    std::string message;
};

template <typename T, typename E = Error>
using Result = std::expected<T, E>;

// fail("...") -> the std::unexpected side of a Result. Keeps call sites short:
//   return dante::fail("not implemented in F0");
inline std::unexpected<Error> fail(std::string message) {
    return std::unexpected<Error>(Error{std::move(message)});
}

}  // namespace dante
