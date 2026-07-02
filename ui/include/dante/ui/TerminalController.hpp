#pragma once

#include <dante/services/IPtyBackend.hpp>
#include <dante/terminal/Screen.hpp>

#include <QObject>
#include <QString>

#include <memory>
#include <thread>

namespace dante {

// Liga um shell (ConPTY) a UI QML: spawna, lê o output num thread e alimenta um term::Screen
// (grade 2D com parser VT completo), envia input tecla-a-tecla. O TerminalView (QQuickPaintedItem)
// renderiza o Screen. F2: terminal real, não mais o preview em modo-linha.
class TerminalController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit TerminalController(QObject* parent = nullptr);
    ~TerminalController() override;

    TerminalController(const TerminalController&) = delete;
    TerminalController& operator=(const TerminalController&) = delete;

    Q_INVOKABLE void start();                             // spawna o shell (idempotente)
    Q_INVOKABLE void send(const QString& text);           // escreve texto cru no shell (ex.: mic)
    Q_INVOKABLE void sendKey(int key, int modifiers, const QString& text); // Qt key -> sequência VT
    Q_INVOKABLE void resize(int cols, int rows);          // grade + PTY (chamado pela view)
    Q_INVOKABLE void clearOutput();                       // limpa a tela (ESC[2J)
    // pede pra UI inserir texto (transcrição do mic) — o TermView roteia p/ send() sem Enter.
    Q_INVOKABLE void requestInsert(const QString& t) { emit insertRequested(t); }

    bool running() const { return running_; }

    // Acesso do TerminalView (C++, mesmo módulo). Lido só durante paint (fase de sync do Qt
    // Quick: a GUI thread está bloqueada), e mutado só na GUI thread — sem corrida.
    const term::Screen& screen() const { return screen_; }

signals:
    void runningChanged();
    void screenChanged(); // grade mudou -> a view chama update()
    void insertRequested(const QString& text);

private:
    void readerLoop();

    std::unique_ptr<IPtyBackend> pty_;
    term::Screen screen_{80, 24};
    bool running_ = false;
    std::thread reader_; // unido no dtor (close() gera EOF)
};

} // namespace dante
