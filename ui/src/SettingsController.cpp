#include "dante/ui/SettingsController.hpp"

namespace dante {

// QSettings usa OrganizationName/ApplicationName definidos no main (DanteCLI / DANTE CLI).
SettingsController::SettingsController(QObject* parent) : QObject(parent) {}

} // namespace dante
