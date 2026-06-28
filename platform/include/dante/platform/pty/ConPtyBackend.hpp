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
// Concorrencia (gotcha do ConPTY): ClosePseudoConsole DESCARTA a saida bufferizada ainda
// nao lida. Com cmd curto (/c echo) o filho sai instantaneamente, entao fechar cedo perde
// o output. Solucao: UMA thread reader que drena via PeekNamedPipe ate a pipe esvaziar (e
// o filho ter saido) e SO ENTAO chama ClosePseudoConsole. read() faz pull da fila.
// ponytail: poll de 5ms quando ocioso (~200 peeks/s/sessao). Trocar por overlapped I/O +
//           WaitForMultipleObjects na F2 (render) p/ a meta de <1% CPU idle.
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
    win::unique_pseudoconsole pseudoConsole_; // fechado pelo reader ao drenar + sair
    win::unique_handle process_;             // handle do shell (cmd/pwsh/...)
    win::unique_handle job_;                 // KILL_ON_JOB_CLOSE: fecha -> mata o filho

    std::jthread reader_;
    std::jthread waiter_;
#endif
};

} // namespace dante
