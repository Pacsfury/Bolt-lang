#include <iostream>
#include <string>

#include "errors.hpp"

void throwError(std::string msg) {
    std::cerr << "\033[31mError!\n" << msg << "\n\033[0m";
    exit(1);
}

void throwWarning(std::string msg) { std::cerr << "\033[33mWarning!\n" << msg << "\n\033[0m"; }

void throwInfo(std::string msg) { std::cout << "\033[36mInformation:\n" << msg << "\n\033[0m"; }