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

struct BinaryOp {
    NodeId left;
    NodeId right;
    char op;
};

struct VarDecl {
    NodeId initializer;
    int name_index;
    bool isconst = false;
};

struct WriteNode {
    std::string str;
};

struct Node {
    std::variant<Literal, BinaryOp, VarDecl, WriteNode> data;
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

AST createAST(std::vector<Token> tokens);