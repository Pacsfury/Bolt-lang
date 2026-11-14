#include <type_traits>
#include <variant>
#include <vector>

#include "../astgen/astgen.hpp"
#include "../../debug/errors.hpp"
#include "structure.hpp"
#include "variables.hpp"

void checkAST(const AST& ast) {
    resetVariables();
    std::vector<bool> integer_expressions(ast.nodes.size(), false);

    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        const Node& n = ast.nodes[index];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, Literal>) {
                    integer_expressions[index] = value.is_integer;
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    if (value.name_index < 0 ||
                        static_cast<size_t>(value.name_index) >= ast.strings.size()) {
                        throwError("Bolt Compile Error: Invalid variable reference");
                    }
                    integer_expressions[index] =
                        checkVariableRead(ast.strings[value.name_index]);
                } else if constexpr (std::is_same_v<T, VarDecl>) {
                    if (value.initializer >= index) {
                        throwError("Bolt Compile Error: Invalid variable initializer");
                    }
                    registervar(ast.strings[value.name_index], value.isconst,
                                integer_expressions[value.initializer]);
                } else if constexpr (std::is_same_v<T, VarChange>) {  // Structure checked by astgen already
                    if (value.initializer >= index) {
                        throwError("Bolt Compile Error: Invalid variable initializer");
                    }
                    checkAssignment(ast.strings[value.name_index],
                                    integer_expressions[value.initializer]);
                } else if constexpr (std::is_same_v<T, iBinOpNode>) {
                    if (value.op != '+' && value.op != '-' && value.op != '*' && value.op != '/') {
                        throwError("Bolt Compile Error: Invalid integer binary operator");
                    }

                    const auto is_expression_node = [&](NodeId id) {
                        return id < index &&
                               (integer_expressions[id] &&
                                (std::holds_alternative<Literal>(ast.nodes[id].data) ||
                                 std::holds_alternative<VarRead>(ast.nodes[id].data) ||
                                 std::holds_alternative<iBinOpNode>(ast.nodes[id].data)));
                    };
                    if (!is_expression_node(value.left) || !is_expression_node(value.right)) {
                        throwError("Bolt Compile Error: Invalid integer binary operation operands");
                    }
                    integer_expressions[index] = true;
                }
            },
            n.data);
    }
}