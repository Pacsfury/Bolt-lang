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

    throwError("Bolt Syntax Error: unrecognized operation");
    return 1;
}


AST createAST(std::vector<Token> tokens) {
    AST ast;
    ParserState state{0, tokens};

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

                VarDecl vardecl{parse_expression(state, ast), ast.add_string(name_token.value)};
                ast.add_node(Node{vardecl});

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
