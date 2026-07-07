#include "commands.hpp"
#include "print.hpp"

#include <glaze/toml.hpp>

#include <format>

auto cmd_init() -> command_result {
    auto paths = project_paths::from_cwd();
    if (fs::exists(paths.cfg)) {
        hoz::println(std::format("✗ .hoz.toml already exists"));
        return {1};
    }

    hoz_config cfg{
        {{"Format", {"cargo fmt --check", {}}},
         {"Clippy", {"cargo clippy --release", {"Format"}}},
         {"Test",   {"cargo test", {"Build"}}},
         {"Build",  {"cargo build", {}}}},
        {{"git", {
            {"pre-commit", {
                hook_entry{.id = "Quality Gate", .step = {"Format", "Clippy"}},
            }},
            {"pre-push", {
                hook_entry{.id = "Full CI", .step = {"Build", "Test"}},
            }},
        }}}
    };

    // Example Claude Code hooks (commented out - uncomment to enable)
    /*
    cfg.hook["claudecode.builtin"]["Stop"] = {
        {
            .id = "Test Verification",
            .step = {"Test"},
            .type = "prompt",
            .prompt = "Verify all tests pass before stopping: $ARGUMENTS"
        }
    };
    */

    std::string toml;
    if (auto ec = glz::write_toml(cfg, toml)) {
        hoz::println(std::format("✗ error generating .hoz.toml"));
        return {1};
    }

    if (auto err = write_file(paths.cfg, toml); !err) {
        hoz::println(std::format("✗ error creating .hoz.toml: {}", err.error().message()));
        return {1};
    }

    hoz::println(std::format("✓ created .hoz.toml"));
    return {0};
}

auto cmd_install() -> command_result {
    auto paths = project_paths::from_cwd();

    if (!fs::exists(paths.cfg)) {
        hoz::println(std::format("✗ .hoz.toml not found (run 'hoz init' first)"));
        return {1};
    }

    auto cfg = load_config(paths.cfg);
    if (!cfg) {
        hoz::println(std::format("✗ {}", cfg.error()));
        return {1};
    }

    hoz::println(std::format("Installing hooks..."));

    // Handle git hooks
    auto git_it = cfg->hook.find("git");
    if (git_it != cfg->hook.end() && !git_it->second.empty()) {
        if (!fs::exists(paths.hook_dir)) {
            hoz::println(std::format("✗ not a git repository (no .git/hooks)"));
            return {1};
        }
        auto result = install_git_hooks(paths, git_it->second, *cfg);
        if (result.exit_code != 0) return result;
    }

    // Handle Claude Code plugin hooks
    auto plugin_it = cfg->hook.find("claudecode.plugin");
    if (plugin_it != cfg->hook.end() && !plugin_it->second.empty()) {
        auto result = install_claude_plugin_hooks(paths, plugin_it->second, *cfg);
        if (result.exit_code != 0) return result;
    }

    // Handle Claude Code builtin hooks
    auto builtin_it = cfg->hook.find("claudecode.builtin");
    if (builtin_it != cfg->hook.end() && !builtin_it->second.empty()) {
        auto result = install_claude_builtin_hooks(paths, builtin_it->second, *cfg);
        if (result.exit_code != 0) return result;
    }

    hoz::println("✓ all hooks installed successfully");
    return {0};
}

auto install_git_hooks(project_paths const& paths,
                       std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                       hoz_config const& cfg) -> command_result {
    hoz::println("  → git hooks");
    
    for (auto const& [hook_name, entries] : hooks) {
        if (hook_name.contains("..") || hook_name.contains('/')) {
            hoz::println("    ! skipping invalid hook name '{}' (contains path separators)", hook_name);
            continue;
        }

        auto hook_path = paths.hook_dir / hook_name;

        std::string script;
        script += std::format("#!/bin/sh\n# hoz hook: {}\n\n", hook_name);

        std::unordered_set<std::string> emitted;
        bool has_error = false;

        for (auto const& entry : entries) {
            script += std::format("echo \"=== {} ===\"\n", entry.id);
            for (auto const& step_id : entry.step) {
                std::unordered_set<std::string> visiting;
                auto result = emit_step(step_id, cfg, emitted, visiting, script);
                if (!result) {
                    hoz::println("    ! warning: {} (hook '{}', entry '{}')",
                                 result.error(), hook_name, entry.id);
                    has_error = true;
                }
            }
        }
        script += '\n';

        if (auto err = write_file(hook_path, script); !err) {
            hoz::println("    ✗ error writing {}: {}", hook_path.c_str(), err.error().message());
            continue;
        }

        std::error_code perm_ec;
        fs::permissions(hook_path,
                        fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                        fs::perm_options::add, perm_ec);
        if (perm_ec) {
            hoz::println("    ! warning: could not make '{}' executable: {}", hook_path.c_str(), perm_ec.message());
        }
        
        auto status = has_error ? " (with errors)" : "";
        hoz::println("    ✓ git hook '{}'{}", hook_name, status);
    }

    return {0};
}

auto install_claude_plugin_hooks(project_paths const& paths,
                                  std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                                  hoz_config const& cfg) -> command_result {
    (void)hooks; // Will be used for script generation in future
    
    hoz::println("  → Claude Code plugin hooks");
    
    // Create plugin directory structure
    auto plugin_dir = paths.claude_plugin_hooks.parent_path();
    std::error_code ec;
    fs::create_directories(plugin_dir, ec);
    if (ec) {
        hoz::println("    ✗ error creating plugin directory: {}", ec.message());
        return {1};
    }

    // Generate hooks.json
    auto json_result = generate_plugin_hooks_json(cfg, "claudecode.plugin");
    if (!json_result) {
        hoz::println("    ✗ error generating plugin hooks JSON: {}", json_result.error());
        return {1};
    }

    if (auto err = write_file(paths.claude_plugin_hooks, *json_result); !err) {
        hoz::println("    ✗ error writing plugin hooks: {}", err.error().message());
        return {1};
    }

    hoz::println("    ✓ Claude Code plugin hooks → {}", paths.claude_plugin_hooks.c_str());
    return {0};
}

auto install_claude_builtin_hooks(project_paths const& paths,
                                    std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                                    hoz_config const& cfg) -> command_result {
    (void)hooks; // Will be used for script generation in future
    
    hoz::println("  → Claude Code builtin hooks");
    
    // Create .claude directory if needed
    auto claude_dir = paths.claude_settings.parent_path();
    std::error_code ec;
    fs::create_directories(claude_dir, ec);
    if (ec) {
        hoz::println("    ✗ error creating .claude directory: {}", ec.message());
        return {1};
    }

    // Generate hooks JSON
    auto json_result = generate_settings_json(cfg, "claudecode.builtin");
    if (!json_result) {
        hoz::println("    ✗ error generating builtin hooks JSON: {}", json_result.error());
        return {1};
    }

    // Check if settings.json already exists
    if (fs::exists(paths.claude_settings)) {
        hoz::println("    ! .claude/settings.json already exists, skipping");
        hoz::println("    ! manually merge the following into your settings.json:");
        hoz::println("{}", *json_result);
        return {0};
    }

    if (auto err = write_file(paths.claude_settings, *json_result); !err) {
        hoz::println("    ✗ error writing settings.json: {}", err.error().message());
        return {1};
    }

    hoz::println("    ✓ Claude Code builtin hooks → {}", paths.claude_settings.c_str());
    return {0};
}

auto cmd_run(std::string_view hook_name, std::optional<std::string_view> filter_id, bool dry_run) -> command_result {
    auto paths = project_paths::from_cwd();

    if (!fs::exists(paths.cfg)) {
        hoz::println(std::format("✗ .hoz.toml not found (run 'hoz init' first)"));
        return {1};
    }

    auto cfg = load_config(paths.cfg);
    if (!cfg) {
        hoz::println(std::format("✗ {}", cfg.error()));
        return {1};
    }

    auto git_it = cfg->hook.find("git");
    if (git_it == cfg->hook.end()) {
        hoz::println("✗ [hook.git] section not found in .hoz.toml");
        return {1};
    }

    auto hook_it = git_it->second.find(std::string(hook_name));
    if (hook_it == git_it->second.end()) {
        hoz::println("✗ hook '{}' not found in [hook.git]", hook_name);
        return {1};
    }

    if (dry_run) {
        hoz::println("→ dry-run for '{}'", hook_name);
    } else {
        hoz::println("→ running '{}'", hook_name);
    }

    int last_code = 0;
    std::unordered_set<std::string> completed;
    bool has_error = false;

    for (auto const& entry : hook_it->second) {
        if (filter_id && entry.id != *filter_id) continue;
        hoz::println("  → [{}]", entry.id);
        for (auto const& step_id : entry.step) {
            std::unordered_set<std::string> visiting;
            auto result = run_step(step_id, *cfg, completed, visiting, dry_run);
            if (!result) {
                hoz::println("  ✗ error: {}", result.error());
                has_error = true;
            } else {
                last_code = *result;
            }
        }
    }

    if (has_error) hoz::println("✗ completed with errors");
    else hoz::println("✓ completed successfully");
    
    return {last_code};
}

auto cmd_version() -> command_result {
    hoz::println("hoz {} ({})", HOZ_VERSION, HOZ_COMMIT_HASH);
    return {0};
}

auto cmd_help() -> command_result {
    hoz::println("hoz {} ({}) - Hook Orchestration for Zero-config", HOZ_VERSION, HOZ_COMMIT_HASH);
    hoz::println();
    hoz::println("USAGE:");
    hoz::println("  hoz <command> [options]");
    hoz::println();
    hoz::println("COMMANDS:");
    hoz::println("  init              Initialize .hoz.toml in current directory");
    hoz::println("  install           Install hooks for configured providers");
    hoz::println("  run <name>        Run a specific hook");
    hoz::println("    --dry-run      Print commands without executing");
    hoz::println("    --id <id>       Run only a specific task group");
    hoz::println("  version, -v       Show version information");
    hoz::println("  help              Show this help message");
    hoz::println();
    hoz::println("PROVIDERS:");
    hoz::println("  git                Git hooks (.git/hooks/)");
    hoz::println("  claudecode.plugin  Claude Code plugin hooks");
    hoz::println("  claudecode.builtin Claude Code builtin hooks");
    hoz::println();
    hoz::println("For more information, see docs/config.md");
    return {0};
}
