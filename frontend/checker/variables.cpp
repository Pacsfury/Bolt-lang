#include <string>
#include <vector>
#include <unordered_map>

#include "../../debug/errors.hpp"
#include "variables.hpp"

void enterScope(ScopeTable& scopes) {
    scopes.emplace_back();
}

void exitScope(ScopeTable& scopes) {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

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

void registervar(ScopeTable& scopes, std::size_t index, const std::string& name, bool isconst) {
    auto& current = scopes.back();
    if (current.find(index) != current.end()) {
        throwError("Bolt Compile Error: Redeclared variable '" + name + "'");
    }
    current[index] = isconst ? VarState::Constant : VarState::Mutable;
}

void checkVariableRead(const ScopeTable& scopes, std::size_t index, const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(index);
        if (found != it->end()) {
            return;
        }
    }
    throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
}

void checkAssignment(const ScopeTable& scopes, std::size_t index, const std::string& name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        const auto found = it->find(index);
        if (found != it->end()) {
            if (found->second == VarState::Constant) {
                throwError("Bolt Compile Error: Cannot modify constant '" + name + "'");
            }
            return;
        }
    }
    throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
}
