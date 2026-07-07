#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

struct project_paths {
    fs::path root;
    fs::path cfg;
    fs::path hook_dir;
    fs::path claude_settings;
    fs::path claude_hooks_dir;
    fs::path claude_plugin_hooks;

    explicit project_paths(fs::path root)
        : root(std::move(root))
        , cfg(this->root / ".hoz.toml")
        , hook_dir(this->root / ".git" / "hooks")
        , claude_settings(this->root / ".claude" / "settings.json")
        , claude_hooks_dir(this->root / ".claude" / "hooks")
        , claude_plugin_hooks(this->root / ".claude" / "plugins" / "hoz" / "hooks" / "hooks.json") {}

    static auto from_cwd() -> project_paths {
        return project_paths(fs::current_path());
    }
};

struct step_def {
    std::string command;
    std::vector<std::string> require;
};

struct hook_entry {
    std::string id;
    std::vector<std::string> step;
    
    // Claude Code specific fields
    std::optional<std::string> matcher;
    std::optional<std::string> type;
    std::optional<std::string> command;
    std::optional<std::vector<std::string>> args;
    std::optional<std::string> if_condition;
    std::optional<std::string> url;
    std::optional<std::unordered_map<std::string, std::string>> headers;
    std::optional<std::vector<std::string>> allowedEnvVars;
    std::optional<std::string> server;
    std::optional<std::string> tool;
    std::optional<std::unordered_map<std::string, std::string>> input;
    std::optional<std::string> prompt;
    std::optional<std::string> model;
    std::optional<int> timeout;
    std::optional<bool> async;
};

struct hoz_config {
    std::unordered_map<std::string, step_def> step;
    std::unordered_map<std::string,
        std::unordered_map<std::string,
            std::vector<hook_entry>
        >
    > hook;
};
