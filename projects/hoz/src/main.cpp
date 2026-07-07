#include "commands.hpp"

#include "print.hpp"
#include <span>
#include <string_view>

auto main(int argc, char const* argv[]) -> int {
    auto args = std::span(argv, std::size_t(argc));

    auto command = args.size() > 1 ? std::string_view(args[1]) : std::string_view("help");

    auto result = [&]() -> command_result {
        if (command == "init") {
            return cmd_init();
        }
        if (command == "install") {
            return cmd_install();
        }
        if (command == "run") {
            if (args.size() < 3) {
                hoz::println("✗ missing hook name");
                return {1};
            }
            bool dry_run = false;
            std::optional<std::string_view> filter_id;
            for (auto i = std::size_t{3}; i < args.size(); ++i) {
                auto arg = std::string_view(args[i]);
                if (arg == "--dry-run") dry_run = true;
                else if (arg == "--id" && i + 1 < args.size()) filter_id = args[++i];
            }
            return cmd_run(args[2], filter_id, dry_run);
        }
        if (command == "version" || command == "-v") {
            return cmd_version();
        }
        if (command == "help") {
            return cmd_help();
        }
        hoz::println("✗ unknown command '{}'", command);
        hoz::println("Usage: hoz <command>  (see 'hoz help')");
        return {1};
    }();

    return result.exit_code;
}
