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
    static constexpr std::array<std::string_view, 21> token_names = {
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
        "write",       // 12
        "const",       // 13
        "read",        // 14
        "scope_begin", // 15
        "scope_end",   // 16
        "equalequal",  // 17
        "notequal",    // 18
        "if_stmt",     // 19
        "while_stmt"   // 20
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
        } else if (auto read = std::get_if<VarRead>(&n.data)) {
            std::cout << "  VarRead: " << ast.strings[read->name_index] << "\n";
        } else if (auto vardecl = std::get_if<VarDecl>(&n.data)) {
            std::cout << "  VarDecl: [name] " << ast.strings[vardecl->name_index] << " [value_node_index] "
                      << vardecl->initializer << " [other] const=" << vardecl->isconst << "\n";
        } else if (auto write = std::get_if<WriteNode>(&n.data)) {
            std::cout << "  WriteNode: " << write->str << "\n";
        } else if (auto binop = std::get_if<iBinOpNode>(&n.data)) {
            std::cout << "  iBinOpNode: " << "[left] "<< binop->left << " [right] " << binop->right << " [op] " << binop->op << "\n";
        } else if (auto whilestmt = std::get_if<WhileStmt>(&n.data)) {
            std::cout << "  WhileStmt: " << "[condition] "<< whilestmt->condition << " [do] " << whilestmt->code << "\n";
        } else if (auto ifstmt = std::get_if<IfStmt>(&n.data)) {
            std::cout << "  IfStmt: [condition] " << ifstmt->condition << " [then] " << ifstmt->then_do
                      << " [else] ";
            if (ifstmt->else_do == static_cast<NodeId>(-1)) {
                std::cout << "none\n";
            } else {
                std::cout << ifstmt->else_do << "\n";
            }
        }
    }
    std::cout << "\n";
}