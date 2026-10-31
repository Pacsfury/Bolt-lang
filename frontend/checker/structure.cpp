#include <type_traits>
#include <variant>
#include <vector>

#include "../astgen/astgen.hpp"
#include "structure.hpp"
#include "variables.hpp"

void checkAST(const AST& ast) {
    static const std::vector<Node> nodes = ast.nodes;

    for (Node n : nodes) {
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, VarDecl>) {  // Structure checked by astgen already
                    registervar(ast.strings[value.name_index], value.isconst);
                }
            },
            n.data);
    }
}