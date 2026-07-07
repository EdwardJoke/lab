#pragma once

#include "config.hpp"

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>

[[nodiscard]] auto read_file(const fs::path& path) -> std::expected<std::string, std::error_code>;

[[nodiscard]] auto write_file(const fs::path& path, std::string_view data)
    -> std::expected<void, std::error_code>;

[[nodiscard]] auto load_config(const fs::path& path) -> std::expected<hoz_config, std::string>;

[[nodiscard]] auto run_command(std::string_view cmd, bool dry_run) -> int;

[[nodiscard]] auto run_step(std::string_view step_id,
                             hoz_config const& cfg,
                             std::unordered_set<std::string>& completed,
                             std::unordered_set<std::string>& visiting,
                             bool dry_run) -> std::expected<int, std::string>;

[[nodiscard]] auto emit_step(std::string_view step_id,
                              hoz_config const& cfg,
                              std::unordered_set<std::string>& emitted,
                              std::unordered_set<std::string>& visiting,
                              std::string& out) -> std::expected<void, std::string>;

[[nodiscard]] auto generate_claude_hook_json(hook_entry const& entry,
                                               std::string_view event_name)
    -> std::expected<std::string, std::string>;

[[nodiscard]] auto generate_plugin_hooks_json(hoz_config const& cfg,
                                               std::string_view provider)
    -> std::expected<std::string, std::string>;

[[nodiscard]] auto generate_settings_json(hoz_config const& cfg,
                                          std::string_view provider)
    -> std::expected<std::string, std::string>;

[[nodiscard]] auto generate_command_hook_script(hook_entry const& entry,
                                                 hoz_config const& cfg,
                                                 std::string_view hook_name)
    -> std::expected<std::string, std::string>;
