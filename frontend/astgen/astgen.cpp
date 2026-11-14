#include <vector>

#include "../../debug/errors.hpp"
#include "../lexer/tokens.hpp"
#include "astgen.hpp"

namespace {
int operator_precedence(tokens type) {
    switch (type) {
        case tokens::plus:
        case tokens::minus:
            return 1;
        case tokens::star:
        case tokens::slash:
            return 2;
        default:
            return -1;
    }
}

char operator_character(tokens type) {
    switch (type) {
        case tokens::plus:
            return '+';
        case tokens::minus:
            return '-';
        case tokens::star:
            return '*';
        case tokens::slash:
            return '/';
        default:
            throwError("Bolt Syntax Error: expected arithmetic operator");
            return '\0';
    }
}

NodeId parse_primary(ParserState& state, AST& ast) {
    Token current = state.consume();

    if (current.type == tokens::c_int) {
        Literal lit{std::stoi(current.value), true};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::c_char) {
        Literal lit{static_cast<unsigned char>(current.value[0]), false};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::c_str) {
        Literal lit{ast.add_string(current.value), false};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::identifier) {
        VarRead read{ast.add_string(current.value)};
        return ast.add_node(Node{read});
    }

    throwError("Bolt Syntax Error: unrecognized operation or value type");
    return 0;
}

NodeId parse_expression(ParserState& state, AST& ast, int minimum_precedence) {
    NodeId left = parse_primary(state, ast);

    while (operator_precedence(state.peek().type) >= minimum_precedence) {
        Token operation = state.consume();
        int precedence = operator_precedence(operation.type);
        NodeId right = parse_expression(state, ast, precedence + 1);
        left = ast.add_node(Node{iBinOpNode{left, right, operator_character(operation.type)}});
    }

    return left;
}
}  // namespace

NodeId parse_expression(ParserState& state, AST& ast) {
    return parse_expression(state, ast, 1);
}

AST createAST(const std::vector<Token>& tokens) {
    AST ast;
    ParserState state{0, tokens};
    std::unordered_map<std::string, std::string> string_values;

    auto remember_string_value = [&](const std::string& name, const Token& value) {
        if (value.type == tokens::c_str || value.type == tokens::c_char) {
            ast.add_string(value.value);
            string_values[name] = value.value;
        } else {
            string_values.erase(name);
        }
    };

    while (state.peek().type != tokens::eof) {
        Token actual = state.peek();

        switch (actual.type) {
            case tokens::var: {
                state.consume();  // consume 'var'
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume();  // consume '='

                Token initializer = state.peek();
                VarDecl vardecl{parse_expression(state, ast), ast.add_string(name_token.value)};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{vardecl});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume();  // consume ';'

                break;
            }
            case tokens::constant: {
                state.consume();  // consume 'const'
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume();  // consume '='

                Token initializer = state.peek();
                VarDecl vardecl{parse_expression(state, ast), ast.add_string(name_token.value), true};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{vardecl});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume();  // consume ';'

                break;
            }
            case tokens::identifier: {
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume();  // consume '='

                Token initializer = state.peek();
                VarChange varchange{parse_expression(state, ast), ast.string_to_index.at(name_token.value)};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{varchange});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume();  // consume ';'

                break;
            }
            case tokens::write: {
                state.consume();  // consume 'write'
                if (state.peek().type != tokens::c_str && state.peek().type != tokens::c_char &&
                    state.peek().type != tokens::identifier) {
                    throwError("Bolt Syntax Error: expected string, char or variable name after 'write'");
                }
                Token output = state.consume();
                std::string node_value;

                if (output.type == tokens::identifier) {
                    auto value = string_values.find(output.value);
                    if (value == string_values.end() ||
                        ast.string_to_index.find(value->second) == ast.string_to_index.end()) {
                        throwError("Bolt Compile Error: variable '" + output.value +
                                   "' does not contain a string or char");
                    }
                    node_value = value->second;
                } else {
                    node_value = output.value;
                }

                WriteNode node{node_value};
                ast.add_node(Node{node});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume();  // consume ';'

                break;
            }
            case tokens::c_int:
            case tokens::c_char:
            case tokens::c_str: {
                parse_expression(state, ast);
                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after expression");
                }
                state.consume();
                break;
            }
            case tokens::eof: {
                goto eof;
                break;
            }
            default:
                state.consume();
                break;
        }
    }
eof:;

    return ast;
}
