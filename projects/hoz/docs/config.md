# Configuration (.hoz.toml)

## Overview

Hoz uses a TOML configuration file (`.hoz.toml`) in the project root. The config defines reusable **steps** and organizes them into **hooks** by provider.

## Steps

Steps are reusable command definitions with optional prerequisites:

```toml
[step.Format]
command = "cargo fmt --check"

[step.Clippy]
command = "cargo clippy --release"
require = ["Format"]

[step.Build]
command = "cargo build"

[step.Test]
command = "cargo test"
require = ["Build"]
```

- `command` — the shell command to execute.
- `require` — array of step IDs that must run first (prerequisites). Steps run only once per invocation even when required by multiple dependents.

## Hooks

Hooks group steps into named task entries under a provider key:

```toml
[[hook.git.pre-commit]]
id = "Quality Gate"
step = ["Format", "Clippy"]

[[hook.git.pre-push]]
id = "Full CI"
step = ["Build", "Test"]
```

- `hook.<provider>.<hook-name>` — the provider (e.g. `git`) and the hook name (e.g. `pre-commit`).
- `[[double-brackets]]` — TOML array-of-tables syntax; each entry is a task group.
- `id` — a label for the task group.
- `step` — array of step IDs to execute in order (prerequisites are resolved automatically).

## Providers

| Provider | Install target        |
|----------|----------------------|
| `git`    | `.git/hooks/<name>`  |
| `claudecode.plugin` | `.claude/plugins/hoz/hooks/hooks.json` |
| `claudecode.builtin` | `.claude/settings.json` |

## Claude Code Hooks

Claude Code hooks support additional fields for hook configuration:

```toml
[[hook.claudecode.plugin.PreToolUse]]
id = "TypeCheck Gate"
matcher = "Write|Edit"
type = "command"
command = "${CLAUDE_PROJECT_DIR}/.claude/hooks/typecheck.sh"
args = []

[[hook.claudecode.builtin.Stop]]
id = "Test Verification"
type = "prompt"
prompt = "Verify all tests pass before stopping: $ARGUMENTS"
model = "claude-3-5-sonnet"
```

### Claude Code Hook Fields

- `matcher` — Pattern to match tool names or events (e.g., `"Write|Edit"`, `"Bash"`)
- `type` — Hook type: `command`, `http`, `mcp_tool`, `prompt`, or `agent`
- `command` — Command to execute (for `command` type)
- `args` — Command arguments array
- `if` — Conditional matcher expression
- `url` — HTTP endpoint URL (for `http` type)
- `headers` — HTTP headers as key-value pairs
- `allowedEnvVars` — Environment variables allowed for interpolation
- `server` — MCP server name (for `mcp_tool` type)
- `tool` — MCP tool name
- `input` — MCP tool input parameters
- `prompt` — Prompt text (for `prompt`/`agent` type)
- `model` — Model to use for evaluation
- `timeout` — Timeout in seconds
- `async` — Run hook in background

### Hook Types

**Command hooks** execute shell scripts:
```toml
[[hook.claudecode.plugin.PostToolUse]]
id = "Lint Check"
type = "command"
command = "${CLAUDE_PROJECT_DIR}/.claude/hooks/lint.sh"
```

**Prompt hooks** use LLM evaluation:
```toml
[[hook.claudecode.builtin.Stop]]
id = "Quality Check"
type = "prompt"
prompt = "Verify code quality: $ARGUMENTS"
```

**HTTP hooks** call web endpoints:
```toml
[[hook.claudecode.plugin.PreToolUse]]
id = "External Validation"
type = "http"
url = "http://localhost:8080/validate"
headers = { "Authorization" = "Bearer $TOKEN" }
allowedEnvVars = ["TOKEN"]
```

## Notes

Steps are defined once via `[step.<id>]` and referenced by name in `step` arrays and `require` arrays. This enables reuse across hooks without duplication.

**Note:** Glaze's TOML parser requires inline-table syntax for map-valued sections like `[step.*]`. Hand-written configs should follow the inline format; run `hoz init` to generate a correct starting config.

**Compiler Compatibility:** hoz uses a custom `print.hpp` compatibility layer to support both C++23 and C++20 compilers. The build system automatically detects compiler support and uses the appropriate standard.
