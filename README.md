# Dante CLI

Multiplexador de terminal **nativo do Windows** — C++23 + Qt6 QML, 100% free/libre (LGPL/MIT). Reconstrução nativa do app macOS (Swift/SwiftUI). Versão 0.0.1 (fase F0: scaffolding).

## Build (macOS, dev)

```bash
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build
./build/app/DanteCLI
```

O executável da GUI é `DanteCLI` (o nome `dante` fica reservado para o CLI helper em `tools/dante`).

`platform/` (ConPTY, DPAPI) compila no macOS apenas como stub — **só funciona de verdade no Windows**.

## Windows

Build pelo CI (`.github/workflows/windows.yml`): Qt 6.6+, gerador padrão do Visual Studio, Debug e Release.

Mapa do projeto e decisões de arquitetura: ver [`CLAUDE.md`](./CLAUDE.md).
