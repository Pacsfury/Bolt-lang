#include <type_traits>
#include <variant>
#include <vector>

#include "../astgen/astgen.hpp"
#include "../../debug/errors.hpp"
#include "structure.hpp"
#include "variables.hpp"

void checkAST(const AST& ast) {
    std::vector<VarState> symbol_table(ast.strings.size(), VarState::Undeclared);

    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        const Node& n = ast.nodes[index];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, VarDecl>) {
                    registervar(symbol_table, value.name_index, ast.strings[value.name_index], value.isconst);
                } else if constexpr (std::is_same_v<T, VarChange>) {
                    checkAssignment(symbol_table, value.name_index, ast.strings[value.name_index]);
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    checkVariableRead(symbol_table, value.name_index, ast.strings[value.name_index]);
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
                } else if constexpr (std::is_same_v<T, ReadNode>) {
                    checkAssignment(symbol_table, value.var_idx, ast.strings[value.var_idx]);
                }
            },
            n.data);
    }
}