#include <string>
#include <vector>

#include "../../debug/errors.hpp"
#include "variables.hpp"

void registervar(std::vector<VarState>& symbol_table, std::size_t index, const std::string& name, bool isconst) {
    if (symbol_table[index] != VarState::Undeclared) {
        throwError("Bolt Compile Error: Redeclared variable '" + name + "'");
    }
    symbol_table[index] = isconst ? VarState::Constant : VarState::Mutable;
}

void checkVariableRead(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name) {
    if (symbol_table[index] == VarState::Undeclared) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }
}

void checkAssignment(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name) {
    if (symbol_table[index] == VarState::Undeclared) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }

    if (symbol_table[index] == VarState::Constant) {
        throwError("Bolt Compile Error: Cannot modify constant '" + name + "'");
    }
}
