#pragma once
#include <vector>

#include "../frontend/astgen/astgen.hpp"
#include "../frontend/lexer/tokens.hpp"

std::string_view tokenToString(tokens type);

void show(std::vector<Token> vec);

void show(const AST& ast);