# hoz — Hook Orchestration for Zero-config

Hoz is a multi-provider hook manager written in C++23. It lets you define reusable steps with prerequisite resolution in a TOML config file, then install or run hooks for git and other providers.

## Build

```bash
cmake -S . -B build
cmake --build build
./build/hoz help
```

## Requirements

- CMake 3.20+
- C++23 compiler recommended (AppleClang 15+, GCC 14+, Clang 18+)
- C++20 minimum with custom print.hpp compatibility layer
- macOS or Linux

## Project Structure

```
CMakeLists.txt           — C++23, vendored Glaze in deps/
deps/glaze/              — Glaze v7.8.4 (TOML serde, header-only)
src/
  config.hpp             — Data model: step_def, hook_entry, hoz_config, project_paths
  resolver.hpp / .cpp    — File I/O, config loading, step resolution (DFS with cycle detection)
  commands.hpp / .cpp    — Subcommands: init, install, run, help
  main.cpp               — Entry point: argument parsing and dispatch
docs/
  config.md              — Config format reference
AGENTS.md                — This file
CLAUDE.md                — Claude Code instructions
```

## Conventions

- C++23 with `CMAKE_CXX_STANDARD_REQUIRED ON`, `CMAKE_CXX_EXTENSIONS OFF`
- Glaze for TOML serialization via aggregate auto-reflection (no manual `glz::meta`)
- No raw `FILE*` — use `std::ifstream`/`std::ofstream` (RAII)
- Pure helpers marked `[[nodiscard]]`
- `std::expected` for fallible operations, `std::println` for output
- Shell commands intentionally use `std::system()` (users need pipes, redirects, `&&`)
- `namespace fs = std::filesystem;` at file scope for brevity

## Glaze TOML Limitation

Glaze's TOML parser only supports inline-table syntax for map-valued sections, not `[section.key]` dotted paths. The generated config from `hoz init` uses the correct format. See `docs/config.md` for details.

## Adding a New Provider

1. Add provider-specific install path logic in `cmd_install()` (e.g. `.claude/hooks/` for claude)
2. Index into `cfg->hook["<provider>"]` instead of hardcoding `"git"`
3. Update `docs/config.md` with the new provider entry
