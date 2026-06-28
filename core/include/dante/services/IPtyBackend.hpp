#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "dante/util/Result.hpp"

namespace dante {

// Pure interface over a pseudo-terminal. ConPTY (Windows) implements this in the
// platform module; outside Windows a stub returns errors. core stays Qt/OS-free.
class IPtyBackend {
public:
    virtual ~IPtyBackend() = default;

    virtual Result<void> start(const std::string& command, std::uint16_t cols,
                               std::uint16_t rows) = 0;
    virtual Result<void> write(std::string_view data) = 0;
    virtual Result<std::string> read() = 0;
    virtual Result<void> resize(std::uint16_t cols, std::uint16_t rows) = 0;
    virtual void close() = 0;
};

}  // namespace dante
