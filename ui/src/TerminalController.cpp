#include "dante/ui/TerminalController.hpp"

#include <dante/platform/PtyFactory.hpp>

#include <QByteArray>
#include <QMetaObject>

#include <string_view>

namespace dante {

TerminalController::TerminalController(QObject* parent) : QObject(parent) {}

TerminalController::~TerminalController() {
    if (pty_) {
        pty_->close(); // gera EOF -> readerLoop sai
    }
    if (reader_.joinable()) {
        reader_.join();
    }
}

void TerminalController::start() {
    if (running_) {
        return;
    }
    pty_ = makePtyBackend();
    if (auto r = pty_->start("cmd.exe", 120, 30); !r) {
        output_ += QString::fromStdString("[falha ao iniciar shell: " + r.error().message + "]\n");
        pty_.reset();
        emit outputChanged();
        return;
    }
    running_ = true;
    emit runningChanged();
    reader_ = std::thread([this] { readerLoop(); });
}

void TerminalController::clearOutput() {
    output_.clear();
    emit outputChanged();
}

void TerminalController::send(const QString& text) {
    if (!pty_) {
        return;
    }
    const QByteArray bytes = text.toUtf8();
    (void)pty_->write(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
}

void TerminalController::readerLoop() {
    for (;;) {
        auto chunk = pty_->read();
        if (!chunk) {
            break; // EOF / sessao fechada
        }
        std::string piece = stripAnsi(*chunk);
        if (piece.empty()) {
            continue;
        }
        // Entrega na UI thread (queued): append + cap de scrollback.
        QMetaObject::invokeMethod(
            this,
            [this, piece = std::move(piece)] {
                output_ += QString::fromUtf8(piece.data(), static_cast<int>(piece.size()));
                if (output_.size() > 200000) {
                    output_ = output_.right(150000);
                }
                emit outputChanged();
            },
            Qt::QueuedConnection);
    }
    QMetaObject::invokeMethod(
        this,
        [this] {
            running_ = false;
            emit runningChanged();
        },
        Qt::QueuedConnection);
}

// Strip minimo de ANSI/VT p/ legibilidade do preview. ponytail: sem grid 2D / cursor
// real (isso e a F2 com parser VT completo); aqui so removemos CSI/OSC e controle.
std::string TerminalController::stripAnsi(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (const unsigned char c : in) {
        switch (parserState_) {
        case 0: // normal
            if (c == 0x1B) {
                parserState_ = 1;
            } else if (c == '\n' || c == '\t') {
                out += static_cast<char>(c);
            } else if (c == '\r') {
                // ignora CR isolado; o \n cuida da quebra
            } else if (c >= 0x20) {
                out += static_cast<char>(c);
            }
            break;
        case 1: // ESC
            if (c == '[') {
                parserState_ = 2; // CSI
            } else if (c == ']') {
                parserState_ = 3; // OSC
            } else {
                parserState_ = 0; // ESC de 2 chars: descarta
            }
            break;
        case 2: // CSI: consome ate byte final (0x40-0x7E)
            if (c >= 0x40 && c <= 0x7E) {
                parserState_ = 0;
            }
            break;
        case 3: // OSC: consome ate BEL (0x07)
            if (c == 0x07) {
                parserState_ = 0;
            }
            break;
        default:
            parserState_ = 0;
            break;
        }
    }
    return out;
}

} // namespace dante
