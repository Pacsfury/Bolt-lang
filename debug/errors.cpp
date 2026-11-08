#include <iostream>
#include <string>
#include <utility>

#include "errors.hpp"

namespace {
std::string errorFile;
}

void setErrorFile(std::string file) {
    errorFile = std::move(file);
}

void throwError(std::string msg) {
    if (!errorFile.empty()) {
        std::cerr << errorFile << ": error: " << msg << "\n";
        exit(1);
    }

    std::cerr << "\033[31mError!\n" << msg << "\n\033[0m";
    exit(1);
}

void throwWarning(std::string msg) { std::cerr << "\033[33mWarning!\n" << msg << "\n\033[0m"; }

void throwInfo(std::string msg) { std::cout << "\033[36mInformation:\n" << msg << "\n\033[0m"; }