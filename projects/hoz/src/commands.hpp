#pragma once

#include "resolver.hpp"

#include <optional>
#include <string_view>

struct command_result {
    int exit_code{};
};

[[nodiscard]] auto cmd_init() -> command_result;
[[nodiscard]] auto cmd_install() -> command_result;
[[nodiscard]] auto cmd_run(std::string_view hook_name, std::optional<std::string_view> filter_id, bool dry_run) -> command_result;
[[nodiscard]] auto cmd_version() -> command_result;
[[nodiscard]] auto cmd_help() -> command_result;

[[nodiscard]] auto install_git_hooks(project_paths const& paths,
                                      std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                                      hoz_config const& cfg) -> command_result;

[[nodiscard]] auto install_claude_plugin_hooks(project_paths const& paths,
                                                 std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                                                 hoz_config const& cfg) -> command_result;

[[nodiscard]] auto install_claude_builtin_hooks(project_paths const& paths,
                                                 std::unordered_map<std::string, std::vector<hook_entry>> const& hooks,
                                                 hoz_config const& cfg) -> command_result;
