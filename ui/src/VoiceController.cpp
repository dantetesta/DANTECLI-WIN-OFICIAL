#include "dante/ui/VoiceController.hpp"

#include <QAudioDevice>
#include <QAudioSource>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMediaDevices>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QtEndian>

namespace dante {

VoiceController::VoiceController(QObject* parent) : QObject(parent) {
    net_ = new QNetworkAccessManager(this);
}

VoiceController::~VoiceController() {
    if (source_ != nullptr) {
        source_->stop();
    }
}

void VoiceController::setState(int s) {
    if (state_ != s) {
        state_ = s;
        emit changed();
    }
}

void VoiceController::fail(const QString& msg) {
    error_ = msg;
    setState(0);
    emit changed();
}

void VoiceController::toggle() {
    if (state_ == 1) {
        stopAndTranscribe();
    } else if (state_ == 0) {
        startRecording();
    }
    // state_ == 2 (transcrevendo): ignora cliques até terminar.
}

void VoiceController::startRecording() {
    error_.clear();
    const QSettings cfg;
    if (cfg.value("groq/apiKey").toString().trimmed().isEmpty()) {
        fail("Configure a Groq API Key em Configurações.");
        return;
    }

    const QAudioDevice dev = QMediaDevices::defaultAudioInput();
    if (dev.isNull()) {
        fail("Nenhum microfone encontrado.");
        return;
    }
    fmt_ = QAudioFormat();
    fmt_.setSampleRate(16000);
    fmt_.setChannelCount(1);
    fmt_.setSampleFormat(QAudioFormat::Int16);
    if (!dev.isFormatSupported(fmt_)) {
        fmt_ = dev.preferredFormat();
    }

    delete source_;
    source_ = new QAudioSource(dev, fmt_, this);
    pcm_.clear();
    io_ = source_->start(); // pull mode: lê do QIODevice retornado
    if (io_.isNull()) {
        fail("Falha ao abrir o microfone (permissão?).");
        return;
    }
    connect(io_, &QIODevice::readyRead, this, [this]() {
        if (!io_.isNull()) {
            pcm_.append(io_->readAll());
        }
    });
    setState(1);
}

// monta um cabeçalho WAV/PCM de 44 bytes em torno dos dados capturados.
QByteArray VoiceController::wavFromPcm() const {
    const quint32 rate = static_cast<quint32>(fmt_.sampleRate());
    const quint16 ch = static_cast<quint16>(fmt_.channelCount());
    const quint16 bits = 16;
    const quint32 byteRate = rate * ch * (bits / 8);
    const quint16 blockAlign = static_cast<quint16>(ch * (bits / 8));
    const quint32 dataLen = static_cast<quint32>(pcm_.size());

    QByteArray h;
    auto put32 = [&h](quint32 v) { quint32 le = qToLittleEndian(v); h.append(reinterpret_cast<const char*>(&le), 4); };
    auto put16 = [&h](quint16 v) { quint16 le = qToLittleEndian(v); h.append(reinterpret_cast<const char*>(&le), 2); };
    h.append("RIFF");
    put32(36 + dataLen);
    h.append("WAVE");
    h.append("fmt ");
    put32(16);
    put16(1); // PCM
    put16(ch);
    put32(rate);
    put32(byteRate);
    put16(blockAlign);
    put16(bits);
    h.append("data");
    put32(dataLen);
    return h + pcm_;
}

void VoiceController::stopAndTranscribe() {
    if (source_ != nullptr) {
        source_->stop();
    }
    if (pcm_.isEmpty()) {
        fail("Nada foi captado pelo microfone.");
        return;
    }
    setState(2);

    const QSettings cfg;
    const QString key = cfg.value("groq/apiKey").toString().trimmed();
    const QString model = cfg.value("groq/model", "whisper-large-v3").toString();

    auto* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart modelPart;
    modelPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"model\""));
    modelPart.setBody(model.toUtf8());
    multi->append(modelPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant("form-data; name=\"file\"; filename=\"audio.wav\""));
    filePart.setBody(wavFromPcm());
    multi->append(filePart);

    QNetworkRequest req(QUrl("https://api.groq.com/openai/v1/audio/transcriptions"));
    req.setRawHeader("Authorization", "Bearer " + key.toUtf8());

    QNetworkReply* reply = net_->post(req, multi);
    multi->setParent(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            fail("Groq: " + reply->errorString());
            return;
        }
        const QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        const QString text = obj.value("text").toString().trimmed();
        error_.clear();
        setState(0);
        if (!text.isEmpty()) {
            emit transcribed(text);
        }
    });
}

} // namespace dante
