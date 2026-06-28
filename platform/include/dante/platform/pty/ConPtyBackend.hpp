#pragma once

#include <dante/services/IPtyBackend.hpp>
#include <dante/util/Result.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>

#ifdef _WIN32
#include <dante/platform/win/ScopedHandle.hpp>
#endif

namespace dante {

// ConPTY (pseudoconsole) real. Sequencia canonica: CreatePipe x2 -> CreatePseudoConsole
// -> CreateProcessW (suspenso) no Job Object KILL_ON_JOB_CLOSE -> ResumeThread.
//
// Concorrencia (gotcha do ConPTY): o conhost segura o pipe de saida MESMO depois do
// processo filho sair, entao um ReadFile bloqueante so destrava apos ClosePseudoConsole.
// Por isso ha duas threads:
//   - reader: ReadFile bloqueante -> fila; encerra em ERROR_BROKEN_PIPE.
//   - waiter: espera o filho sair -> ClosePseudoConsole (destrava o reader).
// read() faz pull da fila (pull-based, conforme a interface do core).
class ConPtyBackend final : public IPtyBackend {
public:
    ConPtyBackend() = default;
    ~ConPtyBackend() override { close(); }

    ConPtyBackend(const ConPtyBackend&) = delete;
    ConPtyBackend& operator=(const ConPtyBackend&) = delete;

    Result<void> start(const std::string& command, std::uint16_t cols, std::uint16_t rows) override;
    Result<void> write(std::string_view data) override;
    Result<std::string> read() override;
    Result<void> resize(std::uint16_t cols, std::uint16_t rows) override;
    void close() override;

private:
#ifdef _WIN32
    void readerLoop();
    void waiterLoop();

    // Fila de saida (reader -> read()). ponytail: mutex+condvar agora; trocar por
    // SPSC lock-free se profiling mostrar contencao (PRD 4.3).
    std::mutex outMtx_;
    std::condition_variable outCv_;
    std::queue<std::string> outQueue_;
    bool finished_ = false;

    std::atomic<bool> closing_{false};

    std::mutex pconMtx_; // serializa ResizePseudoConsole vs ClosePseudoConsole

    // Recursos declarados ANTES das threads: na destruicao, as threads (declaradas
    // por ultimo) sao unidas primeiro, com os handles ainda vivos.
    win::unique_handle inputWrite_;          // escrevemos input do usuario aqui
    win::unique_handle outputRead_;          // lemos output do shell aqui
    win::unique_pseudoconsole pseudoConsole_; // fechado pelo waiter no fim do filho
    win::unique_handle process_;             // handle do shell (cmd/pwsh/...)
    win::unique_handle job_;                 // KILL_ON_JOB_CLOSE: fecha -> mata o filho

    std::jthread reader_;
    std::jthread waiter_;
#endif
};

} // namespace dante
