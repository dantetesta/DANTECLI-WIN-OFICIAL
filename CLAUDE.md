# Dante CLI (Windows) — mapa vivo

**Versão:** 0.0.2 — fase **F1 (ConPTY)**.
**Stack:** C++23 · Qt6 QML (Quick + QuickControls2) · 100% free/libre (libs LGPL/MIT/domínio público, Qt em **link dinâmico** LGPL) · **SEM WebView2/WebEngine**.
**Alvo:** **Windows 10 (1809+ / build 17763) e Windows 11** · x64 + ARM64. ConPTY (`CreatePseudoConsole`) exige 1809+ — fixado em `_WIN32_WINNT=0x0A00`. DPAPI/Job Objects/PeekNamedPipe são todos Win10+.

## O que o app faz
Multiplexador de terminal **nativo do Windows**: várias sessões de shell em abas/painéis, orquestração de agentes num **canvas**, segredos e histórico persistidos localmente. É a reconstrução do app macOS (Swift/SwiftUI) com **paridade conceitual** — mesma ideia de produto, implementação nativa Win32 + Qt.

## Arquitetura (dependência ONE-WAY estrita)
```
app  →  ui  →  { persistence, platform }  →  core
```
`core` não depende de ninguém. Nunca inverter ou criar atalho lateral.

| Módulo        | Target / alias                         | Depende de                         | Papel |
|---------------|----------------------------------------|------------------------------------|-------|
| `core/`       | `dante_core` / `dante::core`           | só stdlib (**ZERO Qt**)            | `Result<T,E>`, `Error`, tipos puros |
| `platform/`   | `dante_platform` / `dante::platform`   | `dante::core` + Qt6::Core          | ConPTY, DPAPI, métricas — Win32 atrás de `#ifdef _WIN32`, stub no else |
| `persistence/`| `dante_persistence` / `dante::persistence` | `dante::core` + Qt6::Core + Qt6::Sql | SQLite, migrações |
| `ui/`         | `dante_ui` / `dante::ui`               | core/platform/persistence + Qt6::Core | controllers C++ (ex.: `AppController`), expostos via **context property** |
| `app/`        | `DanteCLI` (executável, **dono do módulo QML** `Dante`) | `dante::ui` + Qt6::Gui/Quick | entrypoint + `qt_add_qml_module` + logging |
| `tools/dante/`| —                                      | —                                  | placeholder CLI Named-Pipe (só README, fora do build) |

`Result<T,E=dante::Error>` = alias de `std::expected`, vive em `core/include/dante/util/Result.hpp`. Sem exceptions em hot path; RAII + `unique_ptr` por padrão. Win32 sempre sob `#ifdef _WIN32` com stub que compila e retorna `Result` de erro no else.

## Decisões travadas (sessão F0 — não reabrir sem motivo)
1. **UI em Qt6 QML, não QtWidgets** — pela fluidez declarativa tipo SwiftUI. QtWidgets proibido na casca.
2. **Stack 100% free/libre** — só LGPL/MIT/domínio público; **Qt linkado dinamicamente** (cumpre LGPL). Zero dependência proprietária.
3. **WebView2 REMOVIDO** do escopo.
4. **Cortado vs app macOS (perda de paridade deliberada):** navegador embutido, vídeo embed (YouTube/Vimeo), portais de automação web. Não voltam sem reabrir esta decisão.
5. **Canvas / orquestração de agentes MANTIDO** — e é **nativo** (QML + core), não web.

## Como buildar
**macOS (dev):**
```bash
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build
./build/app/DanteCLI
```
`platform/` (ConPTY/DPAPI) compila no mac via stub mas **só funciona de verdade no Windows**.

**Windows:** via CI (`.github/workflows/windows.yml`) — Qt 6.6+, gerador padrão do VS, Debug e Release.

## Roadmap (F0..F9, resumido)
- **F0** — scaffolding que compila **e roda** (verificado no mac: build verde, teste do core passa, janela QML abre viva). Interfaces puras + stubs.
- **F1** — ConPTY real: `CreatePseudoConsole` + `CreateProcessW` (sample canônico do MS) + Job Object `KILL_ON_JOB_CLOSE` + reader (`ReadFile` bloqueante) / waiter `jthread`. RAII em todo `HANDLE`/`HPCON`. CI Windows valida **plumbing** (spawn real, pipe de saída recebendo VT, write/resize/close, teardown limpo). **Conteúdo de tela / shell interativo = validar no Windows interativo** (runner headless do Actions não renderiza conteúdo — ver gotcha). *(atual)*
- **F2** — UI de terminal (render + input) em QML.
- **F3** — abas/painéis/multiplexação.
- **F4** — persistência SQLite migração-safe (sessões, histórico).
- **F5** — segredos via DPAPI (lazy).
- **F6** — canvas de orquestração de agentes (LOD/zoom). *(cortado: nada de web embed)*
- **F7** — orquestração fail-closed entre agentes.
- **F8** — métricas/observabilidade (memória = PrivateUsage).
- **F9** — empacotamento/release Windows.
*(Cortes permanentes: navegador embutido, vídeo embed, portais de automação web — ver decisão 4.)*

## Gotchas críticos (tratar como testes de regressão)
- **Sessão viva desacoplada da UI/registry** — a sessão (ConPTY) sobrevive ao fechar/reabrir a view; nada de amarrar lifetime de processo ao componente QML.
- **Persistência migração-safe** — schema versionado; nunca apagar/recriar dados do usuário num upgrade.
- **Links só `http`/`https`** — qualquer outro esquema é rejeitado (sem `file:`, `javascript:`, etc.).
- **Orquestração fail-closed** — na dúvida, negar/parar; nunca executar agente em estado ambíguo.
- **Canvas LOD: zoom = `scale`, não reparent** — zoom muda escala/level-of-detail, jamais re-hierarquiza nós (reparent quebra estado/seleção).
- **Segredos lazy via DPAPI** — só decifra no ponto de uso; nada de segredo em claro em memória mais do que o necessário.
- **Memória = PrivateUsage** — métrica de RAM é PrivateUsage (não WorkingSet).
- **IDs únicos** — toda sessão/nó/aba tem id único estável; não reusar ids.

### Gotchas de plataforma / build (descobertos na construção)
- **ConPTY: o conhost segura o pipe de saída mesmo após o filho sair (F1)** — `ReadFile` bloqueante **não** retorna no fim do processo. Solução: uma thread *waiter* espera o processo sair (`WaitForSingleObject`) + graça (~300ms p/ flush) e só então `ClosePseudoConsole`, que destrava o *reader* (`ERROR_BROKEN_PIPE`). Fechar as pontas do filho (inputRead/outputWrite) **depois** do `CreateProcess` (ordem do sample MS). Job Object `KILL_ON_JOB_CLOSE` mata a árvore no teardown.
- **ConPTY no CI headless do GitHub Actions renderiza só FRAMING, não conteúdo (F1)** — numa sessão não-interativa, o conhost emite mode-set/clear/title mas **não** o texto de tela do shell, e shells interativos saem na hora (stdin EOF). Validado: o setup segue o sample canônico (testado: `PeekNamedPipe` cega pro streaming → usar `ReadFile` bloqueante; `CREATE_SUSPENDED`, ordem de handles e Job Object **não** eram a causa). **Não é bug do código** — é limitação do ambiente. O CI testa plumbing; o terminal real valida no Windows interativo.
- **Módulo QML pertence ao executável, não a lib estática** — recursos QML (qmlcache) são descartados no link de `static lib` → runtime falha com *"Module Dante contains no type named Main"*. Solução: `qt_add_qml_module` fica no alvo do **executável** (`DanteCLI`); os controllers C++ ficam em `ui/` e entram por **context property** (`App`), sem precisar ser tipos QML.
- **Nome do executável ≠ URI do módulo QML (FS case-insensitive)** — o URI `Dante` cria a pasta `build/app/Dante/`; em macOS/Windows (case-insensitive) ela colide com um exe chamado `dante` → linker falha com **EISDIR**. Por isso o exe é **`DanteCLI`** e o nome `dante` fica para o CLI helper.

## Build / distribuição
- CI Windows empacota (Release): `windeployqt` + `DanteCLI-portatil.zip` + instalador Inno (`installer/dante.iss`), publicados como artefato `DanteCLI-windows`. Baixar com `gh run download <id> -n DanteCLI-windows`.

## Backlog aberto
- **Preview de terminal ligado (atual):** `TerminalController` (ui) ↔ ConPTY via `makePtyBackend()`; QML em **modo linha** (TextField + Enter) com **strip de ANSI simples**. NÃO é terminal real ainda.
- **F2 real:** parser VT + grid 2D + render (QPainter/QRhi) + input char-a-char + cores. Substitui o strip/modo-linha do preview.
- ConPTY: drenar trailing-output antes do `ClosePseudoConsole` (hoje há janela mínima de corte); migrar a fila p/ SPSC lock-free se profiling pedir.
