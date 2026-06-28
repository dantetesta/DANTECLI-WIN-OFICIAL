#include <dante/platform/pty/ConPtyBackend.hpp>

#include <cstdio> // DIAG F1: instrumentacao temporaria do reader
#include <string>
#include <vector>

namespace dante {

#ifdef _WIN32

namespace {

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) {
        return {};
    }
    const int n = ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) {
        return {};
    }
    std::wstring w(static_cast<std::size_t>(n), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

} // namespace

Result<void> ConPtyBackend::start(const std::string& command, std::uint16_t cols, std::uint16_t rows) {
    if (pseudoConsole_.valid()) {
        return fail("ConPTY: ja iniciado");
    }

    // 1. Dois pipes anonimos (nao herdaveis — ConPTY duplica internamente).
    win::unique_handle inputRead, inputWrite, outputRead, outputWrite;
    if (!::CreatePipe(inputRead.put(), inputWrite.put(), nullptr, 0)) {
        return fail("ConPTY: CreatePipe(input) falhou");
    }
    if (!::CreatePipe(outputRead.put(), outputWrite.put(), nullptr, 0)) {
        return fail("ConPTY: CreatePipe(output) falhou");
    }

    // 2. Pseudoconsole. O filho le de inputRead e escreve em outputWrite.
    const COORD size{static_cast<SHORT>(cols), static_cast<SHORT>(rows)};
    HPCON hpc = nullptr;
    if (FAILED(::CreatePseudoConsole(size, inputRead.get(), outputWrite.get(), 0, &hpc))) {
        return fail("ConPTY: CreatePseudoConsole falhou");
    }
    win::unique_pseudoconsole pcon(hpc);

    // 4. Lista de atributos com o pseudoconsole.
    SIZE_T attrSize = 0;
    ::InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
    std::vector<char> attrBuffer(attrSize);
    auto* attrList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attrBuffer.data());
    if (!::InitializeProcThreadAttributeList(attrList, 1, 0, &attrSize)) {
        return fail("ConPTY: InitializeProcThreadAttributeList falhou");
    }
    if (!::UpdateProcThreadAttribute(attrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, hpc,
                                     sizeof(hpc), nullptr, nullptr)) {
        ::DeleteProcThreadAttributeList(attrList);
        return fail("ConPTY: UpdateProcThreadAttribute falhou");
    }

    // 5. DIAG F1: Job Object removido temporariamente (sample oficial nao usa) p/ checar
    //    se ele interferia na sessao do pseudoconsole. Re-adicionar depois.
    win::unique_handle job;

    // 6. Cria o processo SUSPENSO, anexa ao job, depois resume — garante que o filho
    //    ja nasce dentro do job (sem janela de escape).
    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.lpAttributeList = attrList;

    std::wstring wcmd = utf8ToWide(command);
    std::vector<wchar_t> cmdLine(wcmd.begin(), wcmd.end());
    cmdLine.push_back(L'\0');

    // SEM CREATE_SUSPENDED: o sample oficial do MS cria o processo normal. (Suspender +
    // resumir parecia interferir na conexao do pseudoconsole — output nao era emitido.)
    PROCESS_INFORMATION pi{};
    const BOOL created = ::CreateProcessW(
        nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
        EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr, &si.StartupInfo, &pi);
    ::DeleteProcThreadAttributeList(attrList);
    if (!created) {
        return fail("ConPTY: CreateProcessW falhou");
    }
    win::unique_handle process(pi.hProcess);
    win::unique_handle thread(pi.hThread);

    // ponytail: AssignProcessToJobObject e best-effort. Em runners ja dentro de um job
    //           sem nesting, pode falhar; o teardown via ClosePseudoConsole ainda funciona.
    if (job.valid()) {
        ::AssignProcessToJobObject(job.get(), pi.hProcess);
    }
    thread.reset();

    // Fecha NOSSAS copias das pontas do filho SO depois do CreateProcess (ordem do sample
    // oficial do MS). Fechar antes fazia o cmd ver stdin EOF e sair em ~15ms.
    inputRead.reset();
    outputWrite.reset();

    // 7. Publica os recursos e sobe as threads.
    pseudoConsole_ = std::move(pcon);
    inputWrite_ = std::move(inputWrite);
    outputRead_ = std::move(outputRead);
    process_ = std::move(process);
    job_ = std::move(job);
    finished_ = false;
    closing_.store(false);

    reader_ = std::jthread([this] { readerLoop(); });
    waiter_ = std::jthread([this] { waiterLoop(); });
    return {};
}

Result<void> ConPtyBackend::write(std::string_view data) {
    if (closing_.load() || !inputWrite_.valid()) {
        return fail("ConPTY: sessao fechada");
    }
    const char* p = data.data();
    DWORD remaining = static_cast<DWORD>(data.size());
    while (remaining > 0) {
        DWORD written = 0;
        if (!::WriteFile(inputWrite_.get(), p, remaining, &written, nullptr) || written == 0) {
            return fail("ConPTY: WriteFile falhou");
        }
        p += written;
        remaining -= written;
    }
    return {};
}

Result<std::string> ConPtyBackend::read() {
    std::unique_lock lk(outMtx_);
    outCv_.wait(lk, [this] { return !outQueue_.empty() || finished_; });
    if (!outQueue_.empty()) {
        std::string chunk = std::move(outQueue_.front());
        outQueue_.pop();
        return chunk;
    }
    return fail("ConPTY: EOF"); // sessao encerrada e fila drenada
}

Result<void> ConPtyBackend::resize(std::uint16_t cols, std::uint16_t rows) {
    std::scoped_lock lk(pconMtx_);
    if (closing_.load() || !pseudoConsole_.valid()) {
        return fail("ConPTY: sessao fechada");
    }
    const COORD size{static_cast<SHORT>(cols), static_cast<SHORT>(rows)};
    if (FAILED(::ResizePseudoConsole(pseudoConsole_.get(), size))) {
        return fail("ConPTY: ResizePseudoConsole falhou");
    }
    return {};
}

void ConPtyBackend::close() {
    bool expected = false;
    if (!closing_.compare_exchange_strong(expected, true)) {
        return; // teardown roda uma unica vez
    }
    // Fechar o handle do job dispara KILL_ON_JOB_CLOSE -> mata o filho (se ainda vivo).
    // O reader ve closing_, drena o que sobrou na pipe e entao chama ClosePseudoConsole.
    // A jthread reader_ e unida na destruicao do backend.
    job_.reset();
}

void ConPtyBackend::readerLoop() {
    // ReadFile BLOQUEANTE (padrao p/ ConPTY). Streama o output conforme o conhost escreve.
    // O EOF vem quando o waiter chama ClosePseudoConsole (conhost fecha a ponta de escrita).
    char buffer[4096];
    std::size_t diagTotal = 0; // DIAG F1
    for (;;) {
        DWORD n = 0;
        if (!::ReadFile(outputRead_.get(), buffer, sizeof(buffer), &n, nullptr) || n == 0) {
            std::fprintf(stderr, "[reader] fim ReadFile gle=%lu total=%zu\n",
                         ::GetLastError(), diagTotal); // DIAG F1
            break;
        }
        diagTotal += n; // DIAG F1
        {
            std::scoped_lock lk(outMtx_);
            outQueue_.emplace(buffer, n);
        }
        outCv_.notify_one();
    }
    {
        std::scoped_lock lk(outMtx_);
        finished_ = true;
    }
    outCv_.notify_all();
}

void ConPtyBackend::waiterLoop() {
    // Espera o shell sair; da uma graca pro conhost flushar o frame final; entao fecha o
    // pseudoconsole (ClosePseudoConsole quebra o ReadFile bloqueante do reader -> EOF).
    ::WaitForSingleObject(process_.get(), INFINITE);
    DWORD ec = 0;
    ::GetExitCodeProcess(process_.get(), &ec);
    ::Sleep(300); // graca pro flush final
    std::fprintf(stderr, "[waiter] child ec=%lu (0x%lX), fechando pseudoconsole\n", ec, ec); // DIAG F1
    std::scoped_lock lk(pconMtx_);
    pseudoConsole_.reset(); // ClosePseudoConsole — unico ponto que fecha o HPCON em runtime
}

#else // ----- fora do Windows nao existe ConPTY -----

Result<void> ConPtyBackend::start(const std::string&, std::uint16_t, std::uint16_t) {
    return fail("PTY indisponivel fora do Windows");
}

Result<void> ConPtyBackend::write(std::string_view) {
    return fail("PTY indisponivel fora do Windows");
}

Result<std::string> ConPtyBackend::read() {
    return fail("PTY indisponivel fora do Windows");
}

Result<void> ConPtyBackend::resize(std::uint16_t, std::uint16_t) {
    return fail("PTY indisponivel fora do Windows");
}

void ConPtyBackend::close() {
    // no-op.
}

#endif

} // namespace dante
