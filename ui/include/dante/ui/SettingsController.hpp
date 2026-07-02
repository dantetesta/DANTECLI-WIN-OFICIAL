#pragma once

#include <dante/platform/secrets/DpapiSecrets.hpp>

#include <QObject>
#include <QSettings>
#include <QString>

namespace dante {

// Preferências do app. Config leve (modelo do Whisper, shell padrão) fica em QSettings
// (plist/registro). A **Groq API Key** é segredo: vai cifrada via DPAPI (gotcha #6, lazy),
// nunca em texto claro no registro. groqApiKey() decifra sob demanda.
class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString groqApiKey READ groqApiKey WRITE setGroqApiKey NOTIFY changed)
    Q_PROPERTY(QString groqModel READ groqModel WRITE setGroqModel NOTIFY changed)
    Q_PROPERTY(QString defaultShell READ defaultShell WRITE setDefaultShell NOTIFY changed)

public:
    explicit SettingsController(QObject* parent = nullptr);

    // Chave da conta Groq usada por store/load; compartilhada com o VoiceController.
    static constexpr const char* kGroqKeySecret = "groq/apiKey";

    QString groqApiKey() const {
        auto r = secrets_.load(kGroqKeySecret);
        return r ? QString::fromStdString(*r) : QString();
    }
    void setGroqApiKey(const QString& v) {
        if (groqApiKey() != v) {
            (void)secrets_.store(kGroqKeySecret, v.toStdString()); // best-effort; Result é [[nodiscard]]
            emit changed();
        }
    }

    QString groqModel() const { return s_.value("groq/model", "whisper-large-v3").toString(); }
    void setGroqModel(const QString& v) { if (groqModel() != v) { s_.setValue("groq/model", v); emit changed(); } }

    QString defaultShell() const { return s_.value("term/shell", "").toString(); }
    void setDefaultShell(const QString& v) { if (defaultShell() != v) { s_.setValue("term/shell", v); emit changed(); } }

signals:
    void changed();

private:
    QSettings s_;
    DpapiSecrets secrets_;
};

} // namespace dante
