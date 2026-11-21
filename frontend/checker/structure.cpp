#include <type_traits>
#include <variant>
#include <vector>

#include "../astgen/astgen.hpp"
#include "../../debug/errors.hpp"
#include "structure.hpp"
#include "variables.hpp"

void checkAST(const AST& ast) {
    ScopeTable scopes;
    enterScope(scopes);
    size_t scope_depth = 0;

    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        const Node& n = ast.nodes[index];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, ScopeBegin>) {
                    enterScope(scopes);
                    ++scope_depth;
                } else if constexpr (std::is_same_v<T, ScopeEnd>) {
                    if (scope_depth == 0) {
                        throwError("Bolt Syntax Error: unmatched '}'");
                    }
                    exitScope(scopes);
                    --scope_depth;
                } else if constexpr (std::is_same_v<T, VarDecl>) {
                    registervar(scopes, value.name_index, ast.strings[value.name_index], value.isconst);
                } else if constexpr (std::is_same_v<T, VarChange>) {
                    checkAssignment(scopes, value.name_index, ast.strings[value.name_index]);
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    checkVariableRead(scopes, value.name_index, ast.strings[value.name_index]);
                } else if constexpr (std::is_same_v<T, WriteNode>) {
                    if (value.variable_index >= 0) {
                        checkVariableRead(scopes, value.variable_index, ast.strings[value.variable_index]);
                    }
                } else if constexpr (std::is_same_v<T, iBinOpNode>) {
                    const auto is_expression = [&](NodeId id) {
                        if (id >= index) {
                            return false;
                        }
                        const auto& data = ast.nodes[id].data;
                        return std::holds_alternative<Literal>(data) ||
                               std::holds_alternative<VarRead>(data) ||
                               std::holds_alternative<iBinOpNode>(data);
                    };
                    if (!is_expression(value.left) || !is_expression(value.right)) {
                        throwError("Bolt Compile Error: Invalid integer binary operation");
                    }
                } else if constexpr (std::is_same_v<T, ReadNode>) {
                    checkAssignment(scopes, value.var_idx, ast.strings[value.var_idx]);
                }
            },
            n.data);
    }

    if (scope_depth != 0) {
        throwError("Bolt Syntax Error: unmatched '{'");
    }
}