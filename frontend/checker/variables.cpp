#include <string>
#include <unordered_map>

#include "../../debug/errors.hpp"

namespace {
std::unordered_map<std::string, bool> compiler_symbol_table;
}  // namespace

void resetVariables() {
    compiler_symbol_table.clear();
}

void registervar(std::string name, bool isconst) {
    auto [iterator, inserted] = compiler_symbol_table.insert({name, isconst});

    if (!inserted) {
        throwError("Bolt Compile Error: Redeclared variable '" + name + "'");
    }
}

void checkVariableRead(const std::string& name) {
    auto it = compiler_symbol_table.find(name);
    if (it == compiler_symbol_table.end()) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }
}

void checkAssignment(const std::string& name) {
    auto it = compiler_symbol_table.find(name);
    if (it == compiler_symbol_table.end()) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }

    if (it->second) {
        throwError("Bolt Compile Error: Cannot modify constant '" + name + "'");
    }
}
