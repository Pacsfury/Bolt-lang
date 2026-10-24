#pragma once
#include <vector>
#include <string_view>
#include <optional>

class ArgsContext {
private:
    std::vector<std::string_view> m_args;

public:
    ArgsContext(int argc, char* argv[]);

    bool hasArg(std::string_view argName) const;

    std::optional<std::string_view> getArg(std::string_view argName) const;
};
