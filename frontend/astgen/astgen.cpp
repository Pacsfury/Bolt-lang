#include <string>
#include <vector>

#include "../../debug/errors.hpp"
#include "../lexer/tokens.hpp"
#include "astgen.hpp"

namespace {
const Token& peek_token(const ParserState& state) {
    static const Token eof_token{tokens::eof, ""};
    return state.index < state.tokens.size() ? state.tokens[state.index] : eof_token;
}

const Token& consume_token(ParserState& state) {
    const Token& token = peek_token(state);
    if (state.index < state.tokens.size()) {
        ++state.index;
    }
    return token;
}

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
    const Token& current = consume_token(state);

    if (current.type == tokens::c_int) {
        Literal lit{std::stoi(std::string(current.value))};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::c_char) {
        Literal lit{static_cast<unsigned char>(current.value[0])};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::c_str) {
        Literal lit{ast.add_string(std::string(current.value))};
        return ast.add_node(Node{lit});
    }
    if (current.type == tokens::identifier) {
        VarRead read{ast.add_string(std::string(current.value))};
        return ast.add_node(Node{read});
    }

    throwError("Bolt Syntax Error: unrecognized operation or value type");
    return 0;
}

NodeId parse_expression(ParserState& state, AST& ast, int minimum_precedence) {
    NodeId left = parse_primary(state, ast);

    while (operator_precedence(peek_token(state).type) >= minimum_precedence) {
        const Token& operation = consume_token(state);
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
    ast.nodes.reserve(tokens.size());
    ast.strings.reserve(tokens.size());
    ParserState state{0, tokens};
    std::vector<int> string_values;
    std::vector<bool> runtime_string_values;

    auto ensure_variable_slot = [&](int name_index) {
        const size_t required_size = static_cast<size_t>(name_index) + 1;
        if (string_values.size() < required_size) {
            string_values.resize(required_size, -1);
            runtime_string_values.resize(required_size, false);
        }
    };

    auto remember_string_value = [&](int name_index, const Token& value) {
        ensure_variable_slot(name_index);
        int string_index = -1;
        bool is_runtime_string = false;
        if (value.type == tokens::c_str || value.type == tokens::c_char) {
            string_index = ast.add_string(std::string(value.value));
        } else if (value.type == tokens::identifier) {
            const auto source = ast.string_to_index.find(std::string(value.value));
            if (source != ast.string_to_index.end() && source->second >= 0 &&
                static_cast<size_t>(source->second) < string_values.size() &&
                string_values[source->second] != -1) {
                string_index = string_values[source->second];
                is_runtime_string = runtime_string_values[source->second];
            }
        }
        string_values[name_index] = string_index;
        runtime_string_values[name_index] = is_runtime_string;
    };

    while (peek_token(state).type != tokens::eof) {
        const Token& actual = peek_token(state);

        switch (actual.type) {
            case tokens::var:
            case tokens::constant: {
                const bool is_const = consume_token(state).type == tokens::constant;
                const Token& name_token = consume_token(state);
                if (name_token.type != tokens::identifier) {
                    throwError("Bolt Syntax Error: expected variable name after declaration");
                }

                if (peek_token(state).type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                consume_token(state);  // consume '='

                const Token& initializer = peek_token(state);
                const int name_index = ast.add_string(std::string(name_token.value));
                VarDecl vardecl{parse_expression(state, ast), name_index, is_const};
                remember_string_value(name_index, initializer);
                ast.add_node(Node{vardecl});

                if (peek_token(state).type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                consume_token(state);  // consume ';'

                break;
            }
            case tokens::identifier: {
                const Token& name_token = consume_token(state);

                if (peek_token(state).type != tokens::equal) {
                    --state.index;
                    parse_expression(state, ast);
                    if (peek_token(state).type != tokens::semi) {
                        throwError("Bolt Syntax Error: expected ';' after expression");
                    }
                    consume_token(state);
                    break;
                }
                consume_token(state);  // consume '='

                const Token& initializer = peek_token(state);
                const auto name = ast.string_to_index.find(std::string(name_token.value));
                if (name == ast.string_to_index.end()) {
                    throwError("Bolt Compile Error: Variable '" + std::string(name_token.value) +
                               "' is used before declaration");
                }
                const int name_index = name->second;
                VarChange varchange{parse_expression(state, ast), name_index};
                remember_string_value(name_index, initializer);
                ast.add_node(Node{varchange});

                if (peek_token(state).type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                consume_token(state);  // consume ';'

                break;
            }
            case tokens::write: {
                consume_token(state);  // consume 'write'
                if (peek_token(state).type != tokens::c_str && peek_token(state).type != tokens::c_char &&
                    peek_token(state).type != tokens::identifier) {
                    throwError("Bolt Syntax Error: expected string, char or variable name after 'write'");
                }
                const Token& output = consume_token(state);
                std::string node_value;
                int output_name_index = -1;

                if (output.type == tokens::identifier) {
                    const auto name = ast.string_to_index.find(std::string(output.value));
                    if (name == ast.string_to_index.end() ||
                        static_cast<size_t>(name->second) >= string_values.size() ||
                        string_values[name->second] == -1) {
                        throwError("Bolt Compile Error: variable '" + std::string(output.value) +
                                   "' does not contain a string or char");
                    }
                    output_name_index = name->second;
                    if (runtime_string_values[name->second]) {
                        WriteNode node{"", name->second};
                        ast.add_node(Node{node});
                    } else {
                        node_value = ast.strings[string_values[name->second]];
                    }
                } else {
                    node_value = output.value;
                }

                if (output.type != tokens::identifier ||
                    !runtime_string_values[output_name_index]) {
                    WriteNode node{node_value};
                    ast.add_node(Node{node});
                }

                if (peek_token(state).type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                consume_token(state);  // consume ';'

                break;
            }
            case tokens::read: {
                consume_token(state);  // consume 'read'
                ReadNode node;
                const Token& name_token = consume_token(state);
                if (name_token.type != tokens::identifier) {
                    throwError("Bolt Syntax Error: expected variable name after 'read'");
                }
                node.var_idx = ast.add_string(std::string(name_token.value));
                ensure_variable_slot(node.var_idx);
                runtime_string_values[node.var_idx] = true;

                if (peek_token(state).type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                consume_token(state);  // consume ';'
                ast.add_node(Node{node});
                break;
            }
            case tokens::c_int:
            case tokens::c_char:
            case tokens::c_str: {
                parse_expression(state, ast);
                if (peek_token(state).type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after expression");
                }
                consume_token(state);
                break;
            }
            case tokens::eof: {
                goto eof;
                break;
            }
            default:
                consume_token(state);
                break;
        }
    }
eof:;

    return ast;
}
