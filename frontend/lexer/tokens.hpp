#pragma once
#include <string_view>
#include <string>
#include <vector>

enum class tokens { var, c_int, plus, minus, star, slash, c_str, identifier, equal, eof, c_char, semi, write, constant, read };

struct Token {
    tokens type;
    std::string_view value;
};

std::vector<Token> tokenize(const std::string& src);