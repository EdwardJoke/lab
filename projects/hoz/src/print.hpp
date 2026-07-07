#pragma once

#include <format>
#include <iostream>
#include <string>

namespace hoz {

// Universal println - always uses std::format
template <typename... Args>
void println(std::format_string<Args...> fmt, Args&&... args) {
    std::cout << std::format(std::move(fmt), std::forward<Args>(args)...) << '\n';
}

// For plain strings, wrap in format_string
inline void println(const std::string& s) {
    std::cout << s << '\n';
}

inline void println(const char* s) {
    std::cout << s << '\n';
}

inline void println() {
    std::cout << '\n';
}

}
