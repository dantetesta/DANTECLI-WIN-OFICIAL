#pragma once

namespace dante {

// Redireciona qDebug/qWarning/... para AppLocalDataLocation/logs/dante.log.
// No Windows isso cai em %LOCALAPPDATA%. Thread-safe.
void installFileLogger();

} // namespace dante
