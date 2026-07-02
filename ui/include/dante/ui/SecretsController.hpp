#pragma once

#include <dante/platform/secrets/DpapiSecrets.hpp>

#include <QObject>
#include <QString>

namespace dante {

// Ponte QML <-> cofre DPAPI para segredos de credenciais (gotcha #6: cifra no ponto de uso,
// decifra lazy). O QML guarda cada campo secreto sob a chave "cred/<id>/<campo>"; o valor em
// claro NUNCA vai pro SQLite/ListModel — só é decifrado ao revelar/conectar.
class SecretsController : public QObject {
    Q_OBJECT
public:
    explicit SecretsController(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE bool store(const QString& key, const QString& value) {
        return secrets_.store(key.toStdString(), value.toStdString()).has_value();
    }
    Q_INVOKABLE QString load(const QString& key) const {
        auto r = secrets_.load(key.toStdString());
        return r ? QString::fromStdString(*r) : QString();
    }
    Q_INVOKABLE void remove(const QString& key) { (void)secrets_.store(key.toStdString(), std::string{}); }

private:
    DpapiSecrets secrets_;
};

} // namespace dante
