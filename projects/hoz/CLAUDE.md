# hoz — Claude Code Instructions

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Key Files

- `src/config.hpp` — data model; start here to understand the config structures
- `src/resolver.hpp` / `src/resolver.cpp` — file I/O and step resolution with DFS prerequisite traversal
- `src/commands.hpp` / `src/commands.cpp` — command implementations
- `src/main.cpp` — entry point, argument parsing

## Conventions

- C++23 preferred, C++20 minimum with compatibility layer
- No exceptions, no RTTI
- Glaze auto-reflection for TOML (no manual `glz::meta` blocks)
- `std::expected` for error returns, custom `hoz::println` for output
- `[[nodiscard]]` on all helpers
- `std::system()` for shell commands (intentional)
- `namespace fs = std::filesystem;` throughout

## Adding a Provider

Edit `cmd_install()` to add a new provider's hook install path, then index into `cfg->hook["<provider>"]`. The `hook` map in `hoz_config` already supports arbitrary provider keys.

## Config Format

See `docs/config.md` for the full TOML format reference.
