#include <dante/platform/pty/ConPtyBackend.hpp>

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

    // 3. ConPTY ja duplicou as pontas do filho — fechamos as nossas copias.
    inputRead.reset();
    outputWrite.reset();

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

    // 5. Job Object com KILL_ON_JOB_CLOSE (limpeza transitiva do processo + filhos).
    win::unique_handle job(::CreateJobObjectW(nullptr, nullptr));
    if (job.valid()) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli{};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        ::SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    }

    // 6. Cria o processo SUSPENSO, anexa ao job, depois resume — garante que o filho
    //    ja nasce dentro do job (sem janela de escape).
    STARTUPINFOEXW si{};
    si.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    si.lpAttributeList = attrList;

    std::wstring wcmd = utf8ToWide(command);
    std::vector<wchar_t> cmdLine(wcmd.begin(), wcmd.end());
    cmdLine.push_back(L'\0');

    PROCESS_INFORMATION pi{};
    const BOOL created = ::CreateProcessW(
        nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
        EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED, nullptr, nullptr, &si.StartupInfo, &pi);
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
    ::ResumeThread(pi.hThread);
    thread.reset();

    // 7. Publica os recursos e sobe as threads.
    pseudoConsole_ = std::move(pcon);
    inputWrite_ = std::move(inputWrite);
    outputRead_ = std::move(outputRead);
    process_ = std::move(process);
    job_ = std::move(job);
    finished_ = false;
    closing_.store(false);

    reader_ = std::jthread([this] { readerLoop(); });
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
    char buffer[4096];
    bool childExited = false;
    int emptyStreak = 0;
    // Fecha o pseudoconsole so quando o filho saiu E a pipe ficou estavelmente vazia
    // (~6 x 5ms). Drenar ANTES de fechar evita o descarte de output pelo conhost.
    constexpr int kEmptyToClose = 6;

    for (;;) {
        if (closing_.load()) {
            childExited = true; // close(): o job ja matou o filho; drena o que sobrou
        }

        DWORD avail = 0;
        if (!::PeekNamedPipe(outputRead_.get(), nullptr, 0, nullptr, &avail, nullptr)) {
            break; // pipe quebrada
        }

        if (avail > 0) {
            const DWORD toRead = avail < sizeof(buffer) ? avail : static_cast<DWORD>(sizeof(buffer));
            DWORD n = 0;
            if (!::ReadFile(outputRead_.get(), buffer, toRead, &n, nullptr) || n == 0) {
                break;
            }
            {
                std::scoped_lock lk(outMtx_);
                outQueue_.emplace(buffer, n);
            }
            outCv_.notify_one();
            emptyStreak = 0;
            continue; // segue drenando enquanto houver bytes (sem dormir)
        }

        // avail == 0: nada pendente agora.
        if (!childExited && ::WaitForSingleObject(process_.get(), 0) == WAIT_OBJECT_0) {
            childExited = true;
        }
        if (childExited && ++emptyStreak >= kEmptyToClose) {
            std::scoped_lock lk(pconMtx_);
            pseudoConsole_.reset(); // ClosePseudoConsole — unico ponto de fecho em runtime
            break;
        }
        ::Sleep(5);
    }

    {
        std::scoped_lock lk(outMtx_);
        finished_ = true;
    }
    outCv_.notify_all();
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
