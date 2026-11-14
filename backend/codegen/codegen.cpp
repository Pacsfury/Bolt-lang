#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "../../frontend/astgen/astgen.hpp"
#include "../../common/bytecode.h"
#include "codegen.hpp"

namespace {
void emit_u8(std::vector<uint8_t>& out, uint8_t v) { out.push_back(v); }

void emit_u32(std::vector<uint8_t>& out, uint32_t v) {
    out.push_back((v >> 24) & 0xFF);
    out.push_back((v >> 16) & 0xFF);
    out.push_back((v >> 8) & 0xFF);
    out.push_back((v >> 0) & 0xFF);
}
}  // namespace

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateBytecode(const AST& ast, const std::string& path) {
    size_t cpoolidx = 0;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> cpool;
    bytecode.reserve(ast.nodes.size() * 8);
    emit_u8(bytecode, 'B');
    emit_u8(bytecode, 'O');
    emit_u8(bytecode, 'L');
    emit_u8(bytecode, 'T');

    for (const auto& node : ast.nodes) {
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, VarDecl>) {
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.name_index));
                } else if constexpr (std::is_same_v<T, VarChange>) {
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.name_index));
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    emit_u8(bytecode, OP_LOAD);
                    emit_u32(bytecode, static_cast<uint32_t>(value.name_index));
                } else if constexpr (std::is_same_v<T, Literal>) {
                    emit_u8(bytecode, OP_PUSH);
                    emit_u32(bytecode, static_cast<uint32_t>(value.value));
                } else if constexpr (std::is_same_v<T, WriteNode>) {
                    if (value.variable_index >= 0) {
                        emit_u8(bytecode, OP_LOAD);
                        emit_u32(bytecode, static_cast<uint32_t>(value.variable_index));
                        emit_u8(bytecode, OP_PRINT_VAR);
                    } else {
                        cpoolidx++;
                        cpool.insert(cpool.end(), value.str.begin(), value.str.end());
                        emit_u8(cpool, 0x0A);

                        emit_u8(bytecode, OP_PRINT);
                        emit_u32(bytecode, cpoolidx);
                    }
                } else if constexpr (std::is_same_v<T, iBinOpNode>) {
                    uint8_t operation = OP_ADD;
                    switch(value.op) {
                        case '+': operation = OP_ADD; break;
                        case '-': operation = OP_SUB; break;
                        case '*': operation = OP_MUL; break;
                        case '/': operation = OP_DIV; break;
                    }
                    emit_u8(bytecode, operation);
                } else if constexpr (std::is_same_v<T, ReadNode>) {
                    emit_u8(bytecode, OP_READ);
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.var_idx));
                }

            },
            node.data);
    }

    emit_u8(bytecode, OP_HALT);
    return {bytecode, cpool};
}