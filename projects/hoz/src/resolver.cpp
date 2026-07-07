#include "resolver.hpp"
#include "print.hpp"

#include <glaze/toml.hpp>
#include <glaze/json.hpp>

#include <cerrno>
#include <cstdlib>
#include <format>
#include <fstream>
#include <system_error>

[[nodiscard]] auto read_file(const fs::path& path) -> std::expected<std::string, std::error_code> {
    auto file = std::ifstream(path, std::ios::binary);
    if (!file) {
        return std::unexpected(std::error_code{errno, std::generic_category()});
    }
    std::string content;
    content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    if (file.bad()) {
        return std::unexpected(std::error_code{errno, std::generic_category()});
    }
    return content;
}

[[nodiscard]] auto write_file(const fs::path& path, std::string_view data)
    -> std::expected<void, std::error_code> {
    auto file = std::ofstream(path, std::ios::binary);
    if (!file) {
        return std::unexpected(std::error_code{errno, std::generic_category()});
    }
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file) {
        return std::unexpected(std::error_code{errno, std::generic_category()});
    }
    return {};
}

[[nodiscard]] auto load_config(const fs::path& path) -> std::expected<hoz_config, std::string> {
    auto content = read_file(path);
    if (!content) {
        return std::unexpected(std::format("error reading {}: {}", path.c_str(), content.error().message()));
    }
    hoz_config cfg{};
    if (auto ec = glz::read_toml(cfg, *content)) {
        auto msg = glz::format_error(ec, *content);
        return std::unexpected(std::format("error parsing .hoz.toml:\n{}", msg));
    }
    return cfg;
}

[[nodiscard]] auto run_command(std::string_view cmd, bool dry_run) -> int {
    if (dry_run) {
        hoz::println("    → {}", cmd);
        return 0;
    }
    hoz::println("    → {}", cmd);
    return std::system(std::string(cmd).c_str());
}

[[nodiscard]] auto run_step(std::string_view step_id,
                             hoz_config const& cfg,
                             std::unordered_set<std::string>& completed,
                             std::unordered_set<std::string>& visiting,
                             bool dry_run) -> std::expected<int, std::string> {
    auto key = std::string(step_id);

    if (completed.contains(key)) return 0;

    if (visiting.contains(key)) {
        return std::unexpected(std::format("circular dependency detected involving '{}'", key));
    }

    auto def_it = cfg.step.find(key);
    if (def_it == cfg.step.end()) {
        return std::unexpected(std::format("step '{}' not defined", key));
    }

    visiting.insert(key);

    for (auto const& prereq : def_it->second.require) {
        auto result = run_step(prereq, cfg, completed, visiting, dry_run);
        if (!result) return result;
    }

    visiting.erase(key);
    completed.insert(key);

    hoz::println("      → {}", key);
    auto code = run_command(def_it->second.command, dry_run);
    return code;
}

[[nodiscard]] auto emit_step(std::string_view step_id,
                              hoz_config const& cfg,
                              std::unordered_set<std::string>& emitted,
                              std::unordered_set<std::string>& visiting,
                              std::string& out) -> std::expected<void, std::string> {
    auto key = std::string(step_id);

    if (emitted.contains(key)) return {};

    if (visiting.contains(key)) {
        return std::unexpected(std::format("circular dependency detected involving '{}'", key));
    }

    auto def_it = cfg.step.find(key);
    if (def_it == cfg.step.end()) {
        return std::unexpected(std::format("step '{}' not defined", key));
    }

    visiting.insert(key);

    for (auto const& prereq : def_it->second.require) {
        auto result = emit_step(prereq, cfg, emitted, visiting, out);
        if (!result) return result;
    }

    visiting.erase(key);
    emitted.insert(key);

    out += std::format("echo \"      → {}\"\n{}\n", key, def_it->second.command);
    return {};
}

[[nodiscard]] auto json_escape(std::string_view s) -> std::string {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

[[nodiscard]] auto generate_claude_hook_json(hook_entry const& entry,
                                               std::string_view event_name)
    -> std::expected<std::string, std::string> {
    (void)event_name; // Suppress unused parameter warning
    std::string json = "{\n";
    
    // Add matcher if present
    if (entry.matcher) {
        json += std::format("  \"matcher\": \"{}\",\n", json_escape(*entry.matcher));
    }
    
    // Add type
    if (entry.type) {
        json += std::format("  \"type\": \"{}\",\n", json_escape(*entry.type));
    }
    
    // Add command fields for command hooks
    if (entry.type == "command" && entry.command) {
        json += std::format("  \"command\": \"{}\",\n", json_escape(*entry.command));
        if (entry.args && !entry.args->empty()) {
            json += "  \"args\": [";
            bool first = true;
            for (auto const& arg : *entry.args) {
                if (!first) json += ", ";
                json += std::format("\"{}\"", json_escape(arg));
                first = false;
            }
            json += "],\n";
        }
        if (entry.if_condition) {
            json += std::format("  \"if\": \"{}\",\n", json_escape(*entry.if_condition));
        }
    }
    
    // Add HTTP fields for HTTP hooks
    if (entry.type == "http" && entry.url) {
        json += std::format("  \"url\": \"{}\",\n", json_escape(*entry.url));
        if (entry.headers && !entry.headers->empty()) {
            json += "  \"headers\": {\n";
            bool first = true;
            for (auto const& [key, value] : *entry.headers) {
                if (!first) json += ",\n";
                json += std::format("    \"{}\": \"{}\"", json_escape(key), json_escape(value));
                first = false;
            }
            json += "\n  },\n";
        }
        if (entry.allowedEnvVars && !entry.allowedEnvVars->empty()) {
            json += "  \"allowedEnvVars\": [";
            bool first = true;
            for (auto const& var : *entry.allowedEnvVars) {
                if (!first) json += ", ";
                json += std::format("\"{}\"", json_escape(var));
                first = false;
            }
            json += "],\n";
        }
    }
    
    // Add MCP tool fields
    if (entry.type == "mcp_tool" && entry.server && entry.tool) {
        json += std::format("  \"server\": \"{}\",\n", json_escape(*entry.server));
        json += std::format("  \"tool\": \"{}\",\n", json_escape(*entry.tool));
        if (entry.input && !entry.input->empty()) {
            json += "  \"input\": {\n";
            bool first = true;
            for (auto const& [key, value] : *entry.input) {
                if (!first) json += ",\n";
                json += std::format("    \"{}\": \"{}\"", json_escape(key), json_escape(value));
                first = false;
            }
            json += "\n  },\n";
        }
    }
    
    // Add prompt/agent fields
    if ((entry.type == "prompt" || entry.type == "agent") && entry.prompt) {
        json += std::format("  \"prompt\": \"{}\",\n", json_escape(*entry.prompt));
        if (entry.model) {
            json += std::format("  \"model\": \"{}\",\n", json_escape(*entry.model));
        }
    }
    
    // Add common fields
    if (entry.timeout) {
        json += std::format("  \"timeout\": {},\n", *entry.timeout);
    }
    if (entry.async) {
        json += std::format("  \"async\": {},\n", *entry.async);
    }
    
    // Remove trailing comma
    if (json.ends_with(",\n")) {
        json.pop_back();
        json.pop_back();
        json += "\n";
    }
    
    json += "}";
    
    return json;
}

[[nodiscard]] auto generate_plugin_hooks_json(hoz_config const& cfg,
                                               std::string_view provider)
    -> std::expected<std::string, std::string> {
    std::string json = "{\n  \"hooks\": {\n";
    
    auto provider_it = cfg.hook.find(std::string(provider));
    if (provider_it == cfg.hook.end()) {
        return std::unexpected(std::format("provider '{}' not found in config", provider));
    }
    
    bool first_event = true;
    for (auto const& [event_name, entries] : provider_it->second) {
        if (!first_event) json += ",\n";
        first_event = false;
        
        json += std::format("    \"{}\": [\n", event_name);
        
        bool first_entry = true;
        for (auto const& entry : entries) {
            if (!first_entry) json += ",\n";
            first_entry = false;
            
            auto hook_json = generate_claude_hook_json(entry, event_name);
            if (!hook_json) {
                return std::unexpected(std::format("error generating hook for '{}': {}", 
                                                   event_name, hook_json.error()));
            }
            
            // Indent the JSON properly
            std::string indented;
            for (char c : *hook_json) {
                indented += c;
                if (c == '\n') indented += "    ";
            }
            // Remove trailing indentation
            while (indented.ends_with("    ")) {
                indented.pop_back();
                indented.pop_back();
                indented.pop_back();
                indented.pop_back();
            }
            
            json += "      " + indented;
        }
        
        json += "\n    ]";
    }
    
    json += "\n  }\n}";
    return json;
}

[[nodiscard]] auto generate_settings_json(hoz_config const& cfg,
                                          std::string_view provider)
    -> std::expected<std::string, std::string> {
    std::string json = "{\n  \"hooks\": {\n";
    
    auto provider_it = cfg.hook.find(std::string(provider));
    if (provider_it == cfg.hook.end()) {
        return std::unexpected(std::format("provider '{}' not found in config", provider));
    }
    
    bool first_event = true;
    for (auto const& [event_name, entries] : provider_it->second) {
        if (!first_event) json += ",\n";
        first_event = false;
        
        json += std::format("    \"{}\": [\n", event_name);
        
        bool first_entry = true;
        for (auto const& entry : entries) {
            if (!first_entry) json += ",\n";
            first_entry = false;
            
            // Generate hook JSON directly without extra nesting
            json += "      {\n";
            
            // Add matcher if present
            if (entry.matcher) {
                json += std::format("        \"matcher\": \"{}\",\n", json_escape(*entry.matcher));
            }
            
            // Add type
            if (entry.type) {
                json += std::format("        \"type\": \"{}\",\n", json_escape(*entry.type));
            }
            
            // Add prompt for prompt/agent hooks
            if ((entry.type == "prompt" || entry.type == "agent") && entry.prompt) {
                json += std::format("        \"prompt\": \"{}\",\n", json_escape(*entry.prompt));
                if (entry.model) {
                    json += std::format("        \"model\": \"{}\",\n", json_escape(*entry.model));
                }
            }
            
            // Remove trailing comma
            if (json.ends_with(",\n")) {
                json.pop_back();
                json.pop_back();
                json += "\n";
            }
            
            json += "      }";
        }
        
        json += "\n    ]";
    }
    
    json += "\n  }\n}";
    return json;
}

[[nodiscard]] auto generate_command_hook_script(hook_entry const& entry,
                                                 hoz_config const& cfg,
                                                 std::string_view hook_name)
    -> std::expected<std::string, std::string> {
    std::string script = "#!/bin/bash\n";
    script += std::format("# Generated by hoz for Claude Code hook: {}\n\n", hook_name);
    
    // Read JSON input
    script += "INPUT=$(cat)\n";
    script += "SESSION_ID=$(echo \"$INPUT\" | jq -r '.session_id // empty')\n";
    script += "CWD=$(echo \"$INPUT\" | jq -r '.cwd // empty')\n\n";
    
    // Execute steps
    std::unordered_set<std::string> completed;
    std::unordered_set<std::string> visiting;
    std::string step_output;
    
    for (auto const& step_id : entry.step) {
        auto result = emit_step(step_id, cfg, completed, visiting, step_output);
        if (!result) {
            return std::unexpected(std::format("error generating step '{}': {}", 
                                               step_id, result.error()));
        }
    }
    
    // Add step execution to script
    script += "# Execute steps\n";
    script += step_output;
    script += "\n";
    
    // Return JSON output with additionalContext
    script += "# Return JSON output\n";
    script += "OUTPUT=\"Steps executed successfully\"\n";
    script += "jq -nc --arg msg \"$OUTPUT\" '{hookSpecificOutput: {hookEventName: \"" + 
               std::string(hook_name) + "\", additionalContext: $msg}}'\n";
    
    return script;
}
