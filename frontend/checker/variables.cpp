#include <string>
#include <unordered_map>

#include "../../debug/errors.hpp"

namespace {
struct VariableInfo {
    bool isconst;
    bool is_integer;
};

std::unordered_map<std::string, VariableInfo> compiler_symbol_table;
}  // namespace

void resetVariables() {
    compiler_symbol_table.clear();
}

void registervar(std::string name, bool isconst, bool is_integer) {
    auto [iterator, inserted] =
        compiler_symbol_table.insert({name, VariableInfo{isconst, is_integer}});

    if (!inserted) {
        throwError("Bolt Compile Error: Redeclared variable '" + name + "'");
    }
}

bool checkVariableRead(const std::string& name) {
    auto it = compiler_symbol_table.find(name);
    if (it == compiler_symbol_table.end()) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }
    return it->second.is_integer;
}

void checkAssignment(const std::string& name, bool is_integer) {
    auto it = compiler_symbol_table.find(name);
    if (it == compiler_symbol_table.end()) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }

    if (it->second.isconst) {
        throwError("Bolt Compile Error: Cannot modify constant '" + name + "'");
    }
    it->second.is_integer = is_integer;
}
