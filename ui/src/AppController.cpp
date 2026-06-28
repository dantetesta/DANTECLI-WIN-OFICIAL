#include "dante/ui/AppController.hpp"

namespace dante {

AppController::AppController(QObject* parent) : QObject(parent) {}

QString AppController::appName() const { return QStringLiteral("DANTE CLI"); }

QString AppController::version() const { return QStringLiteral("0.0.1 (F0)"); }

} // namespace dante
