#include "dante/ui/TerminalController.hpp"

#include <dante/platform/PtyFactory.hpp>

#include <QByteArray>
#include <QMetaObject>

#include <cstdint>
#include <string>
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
    if (auto r = pty_->start("cmd.exe", static_cast<std::uint16_t>(screen_.cols()),
                             static_cast<std::uint16_t>(screen_.rows()));
        !r) {
#ifdef _WIN32
        screen_.feed("\x1b[31m[falha ao iniciar shell: " + r.error().message + "]\x1b[0m\r\n");
#else
        // Dev no mac: sem ConPTY. Banner colorido pra validar o renderer (selftest/screenshot).
        screen_.feed("\x1b[1;36mDANTE CLI\x1b[0m \x1b[90m— terminal (preview no mac)\x1b[0m\r\n"
                     "ConPTY roda \x1b[1msó no Windows\x1b[0m. Paleta: "
                     "\x1b[31m R \x1b[32m G \x1b[33m Y \x1b[34m B \x1b[35m M \x1b[36m C \x1b[0m\r\n"
                     "\x1b[42;30m ok \x1b[0m grade 2D + cursor renderizando.\r\n> ");
#endif
        pty_.reset();
        emit screenChanged();
        return;
    }
    running_ = true;
    emit runningChanged();
    reader_ = std::thread([this] { readerLoop(); });
}

void TerminalController::clearOutput() {
    screen_.feed("\x1b[2J\x1b[H");
    emit screenChanged();
}

void TerminalController::send(const QString& text) {
    if (!pty_) {
        return;
    }
    const QByteArray bytes = text.toUtf8();
    (void)pty_->write(std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())));
}

void TerminalController::resize(int cols, int rows) {
    if (cols < 1 || rows < 1) {
        return;
    }
    screen_.resize(cols, rows);
    if (pty_) {
        // ponytail: sem debounce ainda. No Windows, ResizePseudoConsole a cada frame do drag é
        //           barulhento; adicionar timer de ~50ms (PRD 4.4) se o profiling pedir.
        (void)pty_->resize(static_cast<std::uint16_t>(cols), static_cast<std::uint16_t>(rows));
    }
    emit screenChanged();
}

// Traduz uma tecla Qt (vinda do QML: Keys.onPressed) na sequência VT que o shell espera.
void TerminalController::sendKey(int key, int modifiers, const QString& text) {
    if (!pty_) {
        return;
    }
    const bool ctrl = (modifiers & Qt::ControlModifier) != 0;
    std::string seq;
    switch (key) {
    case Qt::Key_Return:
    case Qt::Key_Enter:     seq = "\r"; break;
    case Qt::Key_Backspace: seq = "\x7f"; break;
    case Qt::Key_Tab:       seq = "\t"; break;
    case Qt::Key_Escape:    seq = "\x1b"; break;
    case Qt::Key_Up:        seq = "\x1b[A"; break;
    case Qt::Key_Down:      seq = "\x1b[B"; break;
    case Qt::Key_Right:     seq = "\x1b[C"; break;
    case Qt::Key_Left:      seq = "\x1b[D"; break;
    case Qt::Key_Home:      seq = "\x1b[H"; break;
    case Qt::Key_End:       seq = "\x1b[F"; break;
    case Qt::Key_PageUp:    seq = "\x1b[5~"; break;
    case Qt::Key_PageDown:  seq = "\x1b[6~"; break;
    case Qt::Key_Delete:    seq = "\x1b[3~"; break;
    default:
        if (ctrl && key >= Qt::Key_A && key <= Qt::Key_Underscore) {
            seq = std::string(1, static_cast<char>(key & 0x1F)); // Ctrl+A..Ctrl+_ -> 0x01..0x1F
        } else if (!text.isEmpty()) {
            const QByteArray u = text.toUtf8();
            seq.assign(u.constData(), static_cast<std::size_t>(u.size()));
        }
        break;
    }
    if (!seq.empty()) {
        (void)pty_->write(seq);
    }
}

void TerminalController::readerLoop() {
    for (;;) {
        auto chunk = pty_->read();
        if (!chunk) {
            break; // EOF / sessão fechada
        }
        // Feed do Screen acontece na GUI thread (queued) -> Screen fica single-threaded.
        QMetaObject::invokeMethod(
            this,
            [this, s = std::move(*chunk)] {
                screen_.feed(s);
                emit screenChanged();
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

} // namespace dante
