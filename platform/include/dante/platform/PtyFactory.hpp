#pragma once

#include <dante/services/IPtyBackend.hpp>

#include <memory>

namespace dante {

// Cria o backend de PTY concreto (ConPTY no Windows; stub fora). A UI usa SO esta
// fabrica + a interface IPtyBackend — assim windows.h nunca entra no codigo Qt.
std::unique_ptr<IPtyBackend> makePtyBackend();

} // namespace dante
