#include <vector>

#include "../../debug/errors.hpp"
#include "../lexer/tokens.hpp"
#include "astgen.hpp"

inline NodeId parse_expression(ParserState& state, AST& ast) {
    Token current = state.peek();

    if (current.type == tokens::c_int) {
        state.consume();
        Literal lit{std::stoi(current.value)};
        return ast.add_node(Node{lit});
    } 
    else if (current.type == tokens::c_char) {
        state.consume();
        Literal lit{(int)(current.value[0])};
        return ast.add_node(Node{lit});
    }
    else if (current.type == tokens::c_str) {
        state.consume();
        Literal lit{(int)(ast.add_string(current.value))};
        return ast.add_node(Node{lit});
    }

    throwError("Bolt Syntax Error: unrecognized operation");
    return 1;
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
                state.consume(); // consume 'var'
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume(); // consume '='

                Token initializer = state.peek();
                VarDecl vardecl{parse_expression(state, ast), ast.add_string(name_token.value)};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{vardecl});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume(); // consume ';'

                break;
            }
            case tokens::constant: {
                state.consume(); // consume 'const'
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume(); // consume '='

                Token initializer = state.peek();
                VarDecl vardecl{parse_expression(state, ast), ast.add_string(name_token.value), true};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{vardecl});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume(); // consume ';'

                break;
            }
            case tokens::identifier: {
                Token name_token = state.consume();

                if (state.peek().type != tokens::equal) {
                    throwError("Bolt Syntax Error: expected '=' after variable name");
                }
                state.consume(); // consume '='

                Token initializer = state.peek();
                VarChange varchange{parse_expression(state, ast), ast.string_to_index.at(name_token.value)};
                remember_string_value(name_token.value, initializer);
                ast.add_node(Node{varchange});

                if (state.peek().type != tokens::semi) {
                    throwError("Bolt Syntax Error: expected ';' after statement");
                }
                state.consume(); // consume ';'

                break;
            }
            case tokens::write: {
                state.consume(); // consume 'write'
                if (state.peek().type != tokens::c_str && state.peek().type != tokens::c_char && state.peek().type != tokens::identifier) {
                    throwError("Bolt Syntax Error: expected string, char or variable name after 'write'");
                }
                Token output = state.consume();
                std::string node_value;

                if (output.type == tokens::identifier) {
                    auto value = string_values.find(output.value);
                    if (value == string_values.end() ||
                        ast.string_to_index.find(value->second) == ast.string_to_index.end()) {
                        throwError("Bolt Compile Error: variable '" + output.value + "' does not contain a string or char");
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
                state.consume(); // consume ';'

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
