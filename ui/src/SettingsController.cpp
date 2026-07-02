#include "dante/ui/SettingsController.hpp"

namespace dante {

// QSettings usa OrganizationName/ApplicationName definidos no main (DanteCLI / DANTE CLI).
SettingsController::SettingsController(QObject* parent) : QObject(parent) {
    // Migração única: versões antigas guardavam a chave em texto claro no QSettings
    // (registro/plist). Se ainda estiver lá, move pro cofre DPAPI e apaga o texto claro.
    const QString legacy = s_.value("groq/apiKey").toString();
    if (!legacy.isEmpty()) {
        secrets_.store(kGroqKeySecret, legacy.toStdString());
        s_.remove("groq/apiKey");
    }
}

} // namespace dante
