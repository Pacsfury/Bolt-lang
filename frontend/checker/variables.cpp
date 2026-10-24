#include <algorithm>
#include <string>
#include <vector>

#include "../../debug/errors.hpp"

void registervar(std::string name) {
    static std::vector<std::string> DEFINEDVARS;

    if (std::find(DEFINEDVARS.begin(), DEFINEDVARS.end(), name) == DEFINEDVARS.end()) {
        DEFINEDVARS.push_back(name);
    } else {
        throwError("Bolt Syntax Error: Redeclared variable '" + name + "'");
    }
}