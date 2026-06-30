#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace dante {

// Preferências do app (chave-valor nativo: plist no mac, registro no Windows).
// Guarda config leve como a API key do Groq (Whisper do mic) — NÃO é o banco de dados do usuário.
class SettingsController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString groqApiKey READ groqApiKey WRITE setGroqApiKey NOTIFY changed)
    Q_PROPERTY(QString groqModel READ groqModel WRITE setGroqModel NOTIFY changed)
    Q_PROPERTY(QString defaultShell READ defaultShell WRITE setDefaultShell NOTIFY changed)

public:
    explicit SettingsController(QObject* parent = nullptr);

    QString groqApiKey() const { return s_.value("groq/apiKey").toString(); }
    void setGroqApiKey(const QString& v) { if (groqApiKey() != v) { s_.setValue("groq/apiKey", v); emit changed(); } }

    QString groqModel() const { return s_.value("groq/model", "whisper-large-v3").toString(); }
    void setGroqModel(const QString& v) { if (groqModel() != v) { s_.setValue("groq/model", v); emit changed(); } }

    QString defaultShell() const { return s_.value("term/shell", "").toString(); }
    void setDefaultShell(const QString& v) { if (defaultShell() != v) { s_.setValue("term/shell", v); emit changed(); } }

signals:
    void changed();

private:
    QSettings s_;
};

} // namespace dante
