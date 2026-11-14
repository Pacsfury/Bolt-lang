#include <type_traits>
#include <variant>
#include <vector>

#include "../astgen/astgen.hpp"
#include "../../debug/errors.hpp"
#include "structure.hpp"
#include "variables.hpp"

void checkAST(const AST& ast) {
    resetVariables();

    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        const Node& n = ast.nodes[index];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, VarDecl>) {
                    registervar(ast.strings[value.name_index], value.isconst);
                } else if constexpr (std::is_same_v<T, VarChange>) {
                    checkAssignment(ast.strings[value.name_index]);
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    checkVariableRead(ast.strings[value.name_index]);
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
                    if ((value.op != '+' && value.op != '-' && value.op != '*' && value.op != '/') ||
                        !is_expression(value.left) || !is_expression(value.right)) {
                        throwError("Bolt Compile Error: Invalid integer binary operation");
                    }
                }
            },
            n.data);
    }
}