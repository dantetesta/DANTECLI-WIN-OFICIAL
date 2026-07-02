#include <dante/platform/secrets/DpapiSecrets.hpp>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dpapi.h> // CryptProtectData / CryptUnprotectData (Crypt32)
#endif

namespace dante {

namespace {

// Um arquivo por segredo em %LOCALAPPDATA%\<Org>\<App>\secrets\<chave>.bin (fora do OneDrive).
// A chave vira nome de arquivo seguro (só alfanumérico); as chaves usadas são controladas
// (ex.: "groq/apiKey"), então colisão não é preocupação prática.
QString secretPath(const std::string& key) {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(base + "/secrets");
    QString safe;
    safe.reserve(static_cast<int>(key.size()));
    for (const char c : key) {
        const QChar q(c);
        safe += (q.isLetterOrNumber() ? q : QChar('_'));
    }
    return base + "/secrets/" + safe + ".bin";
}

} // namespace

#ifdef _WIN32

// gotcha #6: DPAPI não abre prompt de ACL (CRYPTPROTECT_UI_FORBIDDEN) — ler LAZY no ponto de uso.
Result<void> DpapiSecrets::store(const std::string& key, const std::string& value) {
    const QString path = secretPath(key);
    if (value.empty()) {
        QFile::remove(path); // valor vazio = limpar o segredo
        return {};
    }
    DATA_BLOB in{static_cast<DWORD>(value.size()),
                 reinterpret_cast<BYTE*>(const_cast<char*>(value.data()))};
    DATA_BLOB out{};
    if (!::CryptProtectData(&in, L"dante", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                            &out)) {
        return fail("DPAPI: CryptProtectData falhou");
    }
    const QByteArray blob(reinterpret_cast<const char*>(out.pbData), static_cast<int>(out.cbData));
    ::LocalFree(out.pbData);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail("DPAPI: nao foi possivel gravar o segredo");
    }
    f.write(blob);
    return {};
}

Result<std::string> DpapiSecrets::load(const std::string& key) const {
    QFile f(secretPath(key));
    if (!f.exists()) {
        return fail("DPAPI: segredo ausente");
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return fail("DPAPI: nao foi possivel ler o segredo");
    }
    const QByteArray blob = f.readAll();
    if (blob.isEmpty()) {
        return std::string{};
    }
    DATA_BLOB in{static_cast<DWORD>(blob.size()),
                 reinterpret_cast<BYTE*>(const_cast<char*>(blob.constData()))};
    DATA_BLOB out{};
    if (!::CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN,
                             &out)) {
        return fail("DPAPI: CryptUnprotectData falhou");
    }
    std::string s(reinterpret_cast<const char*>(out.pbData), out.cbData);
    ::LocalFree(out.pbData);
    return s;
}

#else // ----- fora do Windows: fallback SÓ p/ dev no mac (base64, NÃO é criptografia) -----

Result<void> DpapiSecrets::store(const std::string& key, const std::string& value) {
    const QString path = secretPath(key);
    if (value.empty()) {
        QFile::remove(path);
        return {};
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return fail("secrets: nao foi possivel gravar (dev)");
    }
    // ponytail: base64 é ofuscação, não segurança — só existe pra o dev no mac rodar o app.
    //           No Windows (o alvo) o segredo é cifrado de verdade via DPAPI acima.
    f.write(QByteArray::fromStdString(value).toBase64());
    return {};
}

Result<std::string> DpapiSecrets::load(const std::string& key) const {
    QFile f(secretPath(key));
    if (!f.exists()) {
        return fail("secrets: ausente (dev)");
    }
    if (!f.open(QIODevice::ReadOnly)) {
        return fail("secrets: nao foi possivel ler (dev)");
    }
    return QByteArray::fromBase64(f.readAll()).toStdString();
}

#endif

} // namespace dante
