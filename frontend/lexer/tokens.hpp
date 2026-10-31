#pragma once
#include <string>
#include <unordered_map>
#include <vector>

enum class tokens { var, c_int, plus, minus, star, slash, c_str, identifier, equal, eof, c_char, semi, write, constant };

struct Token {
    tokens type;
    std::string value;
};

std::vector<Token> tokenize(const std::string& src);