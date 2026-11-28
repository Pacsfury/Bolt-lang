#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../lexer/tokens.hpp"

using NodeId = uint32_t;

struct Literal {
    int value;
};

struct VarRead {
    int name_index;
    int runtime_index = -1;
};

struct VarDecl {
    NodeId initializer;
    int name_index;
    bool isconst = false;
    int runtime_index = -1;
};

struct VarChange {
    NodeId initializer;
    int name_index;
    int runtime_index = -1;
};

struct WriteNode {
    std::string str;
    int variable_index = -1;
    int runtime_index = -1;
};

struct iBinOpNode {
    NodeId left;
    NodeId right;
    char op;
};

struct ReadNode {
    NodeId var_idx;
    int runtime_index = -1;
};

struct ScopeBegin {};
struct ScopeEnd {};

struct IfStmt {
    NodeId condition;
    NodeId then_do;
    NodeId else_do;
};

struct WhileStmt {
    NodeId condition;
    NodeId code;
};

struct Node {
    std::variant<Literal, VarRead, VarDecl, WriteNode, VarChange, iBinOpNode, ReadNode, ScopeBegin, ScopeEnd, IfStmt, WhileStmt> data;
};

struct AST {
    std::unordered_map<std::string, int> string_to_index;
    std::vector<Node> nodes;
    std::vector<std::string> strings;

    NodeId add_node(Node n) {
        nodes.push_back(n);
        return static_cast<NodeId>(nodes.size() - 1);
    }

    int add_string(const std::string& s) {
        auto it = string_to_index.find(s);
        if (it != string_to_index.end()) {
            return it->second;
        }
        strings.push_back(s);
        int index = strings.size() - 1;
        string_to_index[s] = index;
        return index;
    }
};

struct ParserState {
    size_t index = 0;
    const std::vector<Token>& tokens;
    std::vector<std::unordered_map<std::string, int>>* runtime_scopes = nullptr;
    int* next_runtime_slot = nullptr;

    Token peek() const {
        if (index >= tokens.size())
            return Token{tokens::eof, ""};
        return tokens[index];
    }

    Token consume() {
        if (index >= tokens.size())
            return Token{tokens::eof, ""};
        return tokens[index++];
    }
};

NodeId parse_expression(ParserState& state, AST& ast);

AST createAST(const std::vector<Token>& tokens);