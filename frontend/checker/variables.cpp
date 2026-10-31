#include <string>
#include <unordered_map>

#include "../../debug/errors.hpp"

static std::unordered_map<std::string, bool> COMPILER_SYMBOL_TABLE;

void registervar(std::string name, bool isconst) {
    auto [iterator, inserted] = COMPILER_SYMBOL_TABLE.insert({name, isconst});

    if (!inserted) {
        throwError("Bolt Compile Error: Redeclared variable '" + name + "'");
    }
}

void checkAssignment(const std::string& name) {
    auto it = COMPILER_SYMBOL_TABLE.find(name);
    
    if (it == COMPILER_SYMBOL_TABLE.end()) {
        throwError("Bolt Compile Error: Variable '" + name + "' is used before declaration");
    }
    
    if (it->second) { // Is it const'
        throwError("Bolt Compile Error: Cannot modify constant '" + name + "'");
    }
}
