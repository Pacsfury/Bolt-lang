#include <array>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "../frontend/astgen/astgen.hpp"
#include "../frontend/lexer/tokens.hpp"
#include "print.hpp"

std::string_view tokenToString(tokens type) {
    static constexpr std::array<std::string_view, 13> token_names = {
        "var",         // 0
        "c_int",       // 1
        "plus",        // 2
        "minus",       // 3
        "star",        // 4
        "slash",       // 5
        "c_str",       // 6
        "identifier",  // 7
        "equal",       // 8
        "eof",         // 9
        "c_char",      // 10
        "semi",        // 11
        "write"        // 12
    };

    auto index = static_cast<size_t>(type);

    if (index < token_names.size()) {
        return token_names[index];
    }
    return "invalid";
}

void show(std::vector<Token> vec) {
    std::cout << "TOKENS:\n";
    for (Token t : vec) {
        std::cout << "    " << tokenToString(t.type) << "[" << t.value << "]\n";
    }
}

void show(const AST& ast) {
    std::cout << "AST:\n";
    for (const auto& n : ast.nodes) {
        if (auto literal = std::get_if<Literal>(&n.data)) {
            std::cout << "  Literal: " << literal->value << "\n";
        } else if (auto binop = std::get_if<BinaryOp>(&n.data)) {
            std::cout << "  Binary Operator: [left] " << binop->left << " [op] " << binop->op << " [right] "
                      << binop->right << "\n";
        } else if (auto vardecl = std::get_if<VarDecl>(&n.data)) {
            std::cout << "  VarDecl: [name] " << ast.strings[vardecl->name_index] << " [value_node_index] "
                      << vardecl->initializer << "\n";
        } else if (auto write = std::get_if<WriteNode>(&n.data)) {
            std::cout << "  WriteNode: " << write->str << "\n";
        }
    }
    std::cout << "\n";
}
