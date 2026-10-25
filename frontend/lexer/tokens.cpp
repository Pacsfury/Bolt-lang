#include <string>
#include <unordered_map>
#include <vector>

#include "tokens.hpp"

std::unordered_map<std::string, int> keyword_mapping = {{"var", 0}, {"write", 0}};

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
                t.type = tokens::slash;
                break;
            case '=':
                t.type = tokens::equal;
                break;
            case ';':
                t.type = tokens::semi;
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
                   actual != '-' && actual != '*' && actual != '/' && actual != '=' && actual != ';') {
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
            } else if (token_len >= 2 && (view.front() == view.back()) &&
                       (view.front() == '"' || view.front() == '\'')) {
                if (token_len == 3) {  // '|" + char + '|"
                    t.type = tokens::c_char;
                } else {
                    t.type = tokens::c_str;
                }
                t.value = src.substr(start + 1, token_len - 2);

            } else {
                t.type = tokens::identifier;
                t.value = src.substr(start, token_len);
            }
            result.push_back(t);
        } else {
            i++;
        }
    }

    result.emplace_back(Token{tokens::eof});

    return result;
}
