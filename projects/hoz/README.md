# hoz — Hook Orchestration for Zero-config

**hoz** is a multi-provider hook manager written in C++23. Define reusable
steps with prerequisite resolution in a TOML config file, then install or
run hooks for git and other providers.

See [`docs/config.md`](docs/config.md) for the config format reference.

## Commands

```
hoz init              Create .hoz.toml in current directory
hoz install           Install hooks to .git/hooks/
hoz run <name>        Run all tasks for a hook
hoz run <name> --id <id>
                      Run only a specific task by id
hoz run <name> --dry-run
                      Show what would run without executing
hoz help              Show this help message
```

Hook commands are executed via `/bin/sh`. Review your `.hoz.toml` before running.

The `install` command writes executable shell scripts into `.git/hooks/`,
making them active hooks for `git commit`, `git push`, etc.

## Build & run

Requires CMake 3.20+ and a C++23 compiler.

```sh
cmake -S . -B build
cmake --build build
./build/hoz help
```

## Dependencies

- [Glaze](https://github.com/stephenberry/glaze) v7.8.4 — TOML serde (vendored in `deps/glaze/`)

## Project layout

```
hoz/
├── CMakeLists.txt       # Build definition (C++23)
├── src/
│   ├── config.hpp       # Data model
│   ├── resolver.hpp/cpp # File I/O, step resolution
│   ├── commands.hpp/cpp # Subcommands
│   └── main.cpp         # Entry point
├── deps/glaze/          # Vendored Glaze (header-only)
├── docs/
│   └── config.md        # Config format reference
└── build/               # Build artifacts
```

## License

MIT
