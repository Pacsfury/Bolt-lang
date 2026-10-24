#include <optional>
#include <string_view>
#include <vector>

#include "arguments.hpp"

ArgsContext::ArgsContext(int argc, char* argv[]) {
    if (argv != nullptr && argc > 0) {
        m_args.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            if (argv[i] != nullptr) {
                m_args.emplace_back(argv[i]);
            }
        }
    }
}

bool ArgsContext::hasArg(std::string_view argName) const {
    for (const auto& arg : m_args) {
        if (arg == argName) {
            return true;
        }
    }
    return false;
}

std::optional<std::string_view> ArgsContext::getArg(std::string_view argName) const {
    for (size_t i = 0; i < m_args.size(); ++i) {
        if (m_args[i] == argName) {
            if (i + 1 < m_args.size()) {
                return m_args[i + 1];
            }
            return std::nullopt;
        }
    }
    return std::nullopt;
}