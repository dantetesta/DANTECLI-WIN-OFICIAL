#include <dante/platform/PtyFactory.hpp>

#include <dante/platform/pty/ConPtyBackend.hpp> // windows.h confinado a este TU

namespace dante {

std::unique_ptr<IPtyBackend> makePtyBackend() {
    return std::make_unique<ConPtyBackend>();
}

} // namespace dante
