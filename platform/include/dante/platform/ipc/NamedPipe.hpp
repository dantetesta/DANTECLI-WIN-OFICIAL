#pragma once

#include <dante/util/Result.hpp>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#ifdef _WIN32
#include <dante/platform/win/ScopedHandle.hpp>
#endif

namespace dante {

// Servidor Named Pipe (\\.\pipe\dante-<name>). Aceita 1 cliente por vez, lê a mensagem, chama o
// handler e escreve a resposta. Message-mode + PIPE_REJECT_REMOTE_CLIENTS (só local). Win32;
// stub fora do Windows. Qt-free (o dante.exe linka isto sem puxar Qt).
class NamedPipeServer {
public:
    using Handler = std::function<std::string(const std::string& request)>;

    NamedPipeServer() = default;
    ~NamedPipeServer();
    NamedPipeServer(const NamedPipeServer&) = delete;
    NamedPipeServer& operator=(const NamedPipeServer&) = delete;

    Result<void> start(const std::string& name, Handler handler);
    void stop();

private:
#ifdef _WIN32
    void loop();
    std::string name_;
    Handler handler_;
    std::atomic<bool> stopping_{false};
    std::thread thread_;
#endif
};

// Cliente one-shot: conecta, envia request, devolve a resposta. Win32; stub fora do Windows.
Result<std::string> pipeClientSend(const std::string& name, const std::string& request);

} // namespace dante
