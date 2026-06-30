#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>

class QAudioSource;
class QIODevice;
class QNetworkAccessManager;

namespace dante {

// Mic -> Groq Whisper. toggle() grava; ao parar, sobe o WAV pra api.groq.com e
// emite transcribed(texto). A chave/modelo vêm do QSettings (Configurações).
class VoiceController : public QObject {
    Q_OBJECT
    Q_PROPERTY(int state READ state NOTIFY changed)      // 0 idle · 1 gravando · 2 transcrevendo
    Q_PROPERTY(QString error READ error NOTIFY changed)

public:
    explicit VoiceController(QObject* parent = nullptr);
    ~VoiceController() override;

    int state() const { return state_; }
    QString error() const { return error_; }

    Q_INVOKABLE void toggle();

signals:
    void changed();
    void transcribed(const QString& text);

private:
    void startRecording();
    void stopAndTranscribe();
    void setState(int s);
    void fail(const QString& msg);
    QByteArray wavFromPcm() const;

    int state_ = 0;
    QString error_;
    QAudioSource* source_ = nullptr;
    QPointer<QIODevice> io_;
    QByteArray pcm_;
    QAudioFormat fmt_;
    QNetworkAccessManager* net_ = nullptr;
};

} // namespace dante
