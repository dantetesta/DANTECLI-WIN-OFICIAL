#include <dante/platform/ipc/NamedPipe.hpp>

#ifdef _WIN32

#include <cctype>

namespace dante {

namespace {
constexpr DWORD kBuf = 8192;
// Nome vira caminho de pipe seguro: só alfanumérico após o prefixo fixo.
std::wstring pipePath(const std::string& name) {
    std::wstring w = L"\\\\.\\pipe\\dante-";
    for (const char c : name) {
        w += std::isalnum(static_cast<unsigned char>(c)) ? static_cast<wchar_t>(c) : L'_';
    }
    return w;
}
} // namespace

NamedPipeServer::~NamedPipeServer() { stop(); }

Result<void> NamedPipeServer::start(const std::string& name, Handler handler) {
    if (thread_.joinable()) {
        return fail("pipe: servidor já iniciado");
    }
    name_ = name;
    handler_ = std::move(handler);
    stopping_.store(false);
    thread_ = std::thread([this] { loop(); });
    return {};
}

void NamedPipeServer::stop() {
    if (!thread_.joinable()) {
        return;
    }
    stopping_.store(true);
    (void)pipeClientSend(name_, "__stop__"); // cliente-fantasma destrava o ConnectNamedPipe
    thread_.join();
}

void NamedPipeServer::loop() {
    const std::wstring path = pipePath(name_);
    while (!stopping_.load()) {
        win::unique_handle pipe(::CreateNamedPipeW(
            path.c_str(), PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
            PIPE_UNLIMITED_INSTANCES, kBuf, kBuf, 0, nullptr));
        if (!pipe.valid()) {
            break;
        }
        const BOOL ok = ::ConnectNamedPipe(pipe.get(), nullptr);
        if (!ok && ::GetLastError() != ERROR_PIPE_CONNECTED) {
            continue;
        }
        if (stopping_.load()) {
            break;
        }
        char buf[kBuf];
        DWORD n = 0;
        if (::ReadFile(pipe.get(), buf, sizeof(buf), &n, nullptr) && n > 0) {
            const std::string req(buf, n);
            const std::string resp = handler_ ? handler_(req) : std::string{};
            DWORD w = 0;
            ::WriteFile(pipe.get(), resp.data(), static_cast<DWORD>(resp.size()), &w, nullptr);
            ::FlushFileBuffers(pipe.get());
        }
        ::DisconnectNamedPipe(pipe.get());
    }
}

Result<std::string> pipeClientSend(const std::string& name, const std::string& request) {
    const std::wstring path = pipePath(name);
    win::unique_handle h(::CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                       OPEN_EXISTING, 0, nullptr));
    if (!h.valid()) {
        return fail("pipe: conexão falhou");
    }
    DWORD mode = PIPE_READMODE_MESSAGE;
    ::SetNamedPipeHandleState(h.get(), &mode, nullptr, nullptr);
    DWORD w = 0;
    if (!::WriteFile(h.get(), request.data(), static_cast<DWORD>(request.size()), &w, nullptr)) {
        return fail("pipe: write falhou");
    }
    char buf[kBuf];
    DWORD n = 0;
    if (!::ReadFile(h.get(), buf, sizeof(buf), &n, nullptr)) {
        return fail("pipe: read falhou");
    }
    return std::string(buf, n);
}

} // namespace dante

#else // ----- fora do Windows: Named Pipe não existe -----

namespace dante {

NamedPipeServer::~NamedPipeServer() = default;

Result<void> NamedPipeServer::start(const std::string&, Handler) {
    return fail("Named Pipe indisponível fora do Windows");
}

void NamedPipeServer::stop() {}

Result<std::string> pipeClientSend(const std::string&, const std::string&) {
    return fail("Named Pipe indisponível fora do Windows");
}

} // namespace dante

#endif
