#pragma once

#include <QObject>
#include <QTimer>

namespace dante {

// Mede CPU%/memória do processo a cada 2s e expõe ao QML (popover de Consumo de Recursos).
class SysStatsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cpu READ cpu NOTIFY changed)   // % de 1 núcleo (pode passar de 100)
    Q_PROPERTY(double memMB READ memMB NOTIFY changed)
    Q_PROPERTY(int cores READ cores CONSTANT)

public:
    explicit SysStatsController(QObject* parent = nullptr);

    double cpu() const { return cpu_; }
    double memMB() const { return memMB_; }
    int cores() const { return cores_; }

signals:
    void changed();

private:
    void tick();

    QTimer timer_;
    double cpu_ = 0.0;
    double memMB_ = 0.0;
    double lastCpuSeconds_ = -1.0;
    qint64 lastMs_ = 0;
    int cores_ = 1;
};

} // namespace dante
