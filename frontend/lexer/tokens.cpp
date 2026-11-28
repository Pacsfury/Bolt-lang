#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

#include "tokens.hpp"

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> result;
    size_t i = 0;
    size_t length = src.size();
    result.reserve(length / 4);

    while (i < length) {
        char actual = src[i];
        Token t;

        if (actual == ' ' || actual == '\t' || actual == '\n' || actual == '\r') {
            i++;
            continue;
        }

        bool is_single_char = true;
        switch (actual) {
            case '+':
                t.type = tokens::plus;
                break;
            case '-':
                t.type = tokens::minus;
                break;
            case '*':
                t.type = tokens::star;
                break;
            case '/':
                if (i + 1 < length && src[i + 1] == '/') {
                    i += 2;
                    while (i < length && src[i] != '\n') {
                        i++;
                    }
                    continue;
                } else if (i + 1 < length && src[i + 1] == '*') {
                    i += 2;
                    while (i + 1 < length && !(src[i] == '*' && src[i + 1] == '/')) {
                        i++;
                    }
                    if (i + 1 < length) {
                        i += 2;
                    }
                    continue;
                }
                t.type = tokens::slash;
                break;
            case '=':
                if (i + 1 < length && src[i + 1] == '=') {
                    t.type = tokens::equalequal;
                    i += 2;
                    result.push_back(t);
                    continue;
                }
                t.type = tokens::equal;
                break;
            case '!':
                if (i + 1 < length && src[i + 1] == '=') {
                    t.type = tokens::notequal;
                    i += 2;
                    result.push_back(t);
                    continue;
                }
                t.type = tokens::equal;
                break;
            case ';':
                t.type = tokens::semi;
                break;
            case '{':
                t.type = tokens::scope_begin;
                break;
            case '}':
                t.type = tokens::scope_end;
                break;
            default:
                is_single_char = false;
                break;
        }

        if (is_single_char) {
            i++;
            result.push_back(t);
            continue;
        }

        size_t start = i;
        if (actual == '"' || actual == '\'') {
            const char quote = actual;
            while (++i < length) {
                if (src[i] == '\\' && i + 1 < length) {
                    ++i;
                } else if (src[i] == quote) {
                    ++i;
                    break;
                }
            }
        } else {
            while (i < length && actual != ' ' && actual != '\t' && actual != '\n' && actual != '\r' && actual != '+' &&
                   actual != '-' && actual != '*' && actual != '/' && actual != '=' && actual != '!' && actual != ';') {
                if (++i >= length) {
                    break;
                }
                actual = src[i];
            }
        }

        size_t token_len = i - start;

        if (token_len > 0) {
            std::string_view view(src.data() + start, token_len);

            if (view == "var") {
                t.type = tokens::var;
            } else if (view == "write") {
                t.type = tokens::write;
            } else if (view == "read") {
                t.type = tokens::read;
            } else if (view == "const") {
                t.type = tokens::constant;
            } else if (view == "if") {
                t.type = tokens::if_stmt;
            } else if (view == "while") {
                t.type = tokens::while_stmt;
            } else if (token_len >= 2 && view.front() == view.back() &&
                       (view.front() == '"' || view.front() == '\'')) {
                if (token_len == 3) {  // '"' + char + '"'
                    t.type = tokens::c_char;
                } else {
                    t.type = tokens::c_str;
                }
                t.value = view.substr(1, token_len - 2);

            } else if (std::all_of(view.begin(), view.end(), [](unsigned char c) {
                           return std::isdigit(c) != 0;
                       })) {
                t.type = tokens::c_int;
                t.value = view;
            } else {
                t.type = tokens::identifier;
                t.value = view;
            }
            result.push_back(t);
        } else {
            i++;
        }
    }

    result.emplace_back(Token{tokens::eof});

    return result;
}
