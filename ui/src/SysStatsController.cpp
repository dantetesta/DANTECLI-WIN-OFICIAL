#include "dante/ui/SysStatsController.hpp"

#include <dante/platform/SysStats.hpp>

#include <QDateTime>

#include <algorithm>
#include <thread>

namespace dante {

SysStatsController::SysStatsController(QObject* parent) : QObject(parent) {
    const unsigned hw = std::thread::hardware_concurrency();
    cores_ = static_cast<int>(hw == 0 ? 1u : hw);
    timer_.setInterval(2000);
    connect(&timer_, &QTimer::timeout, this, &SysStatsController::tick);
    tick();
    timer_.start();
}

void SysStatsController::tick() {
    const SysSample s = sysSample();
    memMB_ = static_cast<double>(s.privateBytes) / (1024.0 * 1024.0);

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastCpuSeconds_ >= 0.0 && now > lastMs_) {
        const double dtSec = static_cast<double>(now - lastMs_) / 1000.0;
        const double dCpu = s.cpuSeconds - lastCpuSeconds_;
        cpu_ = dtSec > 0.0 ? std::max(0.0, (dCpu / dtSec) * 100.0) : 0.0;
    }
    lastCpuSeconds_ = s.cpuSeconds;
    lastMs_ = now;
    emit changed();
}

} // namespace dante
