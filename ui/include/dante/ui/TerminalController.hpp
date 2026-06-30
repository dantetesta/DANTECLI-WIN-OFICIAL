#pragma once

#include <dante/services/IPtyBackend.hpp>

#include <QObject>
#include <QString>

#include <memory>
#include <string>
#include <thread>

namespace dante {

// Liga um shell (ConPTY) a UI QML: spawna, le output num thread -> sinal (na UI thread),
// e envia o que o usuario digita. Preview da F1/F2 — strip de ANSI simples (sem grid).
class TerminalController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
    explicit TerminalController(QObject* parent = nullptr);
    ~TerminalController() override;

    TerminalController(const TerminalController&) = delete;
    TerminalController& operator=(const TerminalController&) = delete;

    Q_INVOKABLE void start();                  // spawna o shell (idempotente)
    Q_INVOKABLE void send(const QString& text); // escreve no shell (ex.: "dir\r")
    Q_INVOKABLE void clearOutput();            // limpa a tela do terminal

    QString output() const { return output_; }
    bool running() const { return running_; }

signals:
    void outputChanged();
    void runningChanged();

private:
    void readerLoop();
    std::string stripAnsi(const std::string& in); // estado de parser (so reader thread)

    std::unique_ptr<IPtyBackend> pty_;
    QString output_;
    bool running_ = false;
    int parserState_ = 0; // 0 normal, 1 esc, 2 csi, 3 osc
    std::thread reader_;  // unido no dtor (close() gera EOF)
};

} // namespace dante
