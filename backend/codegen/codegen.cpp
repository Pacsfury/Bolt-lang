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

void patch_u32(std::vector<uint8_t>& out, size_t offset, uint32_t value) {
    out[offset] = (value >> 24) & 0xFF;
    out[offset + 1] = (value >> 16) & 0xFF;
    out[offset + 2] = (value >> 8) & 0xFF;
    out[offset + 3] = value & 0xFF;
}

uint32_t code_offset(const std::vector<uint8_t>& bytecode) {
    return static_cast<uint32_t>(bytecode.size() - 4);
}

struct IfPatch {
    size_t then_end;
    size_t else_end;
    size_t condition_target;
    size_t end_target = 0;
};
}  // namespace

std::pair<std::vector<uint8_t>, std::vector<uint8_t>> generateBytecode(const AST& ast) {
    size_t cpoolidx = 0;
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> cpool;
    bytecode.reserve(ast.nodes.size() * 8);
    emit_u8(bytecode, 'B');
    emit_u8(bytecode, 'O');
    emit_u8(bytecode, 'L');
    emit_u8(bytecode, 'T');

    std::vector<size_t> scope_end_for_begin(ast.nodes.size(), ast.nodes.size());
    std::vector<size_t> scope_begins;
    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        if (std::holds_alternative<ScopeBegin>(ast.nodes[index].data)) {
            scope_begins.push_back(index);
        } else if (std::holds_alternative<ScopeEnd>(ast.nodes[index].data) && !scope_begins.empty()) {
            scope_end_for_begin[scope_begins.back()] = index;
            scope_begins.pop_back();
        }
    }
    std::vector<IfPatch> if_patches;

    for (size_t index = 0; index < ast.nodes.size(); ++index) {
        const auto& node = ast.nodes[index];
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;

                if constexpr (std::is_same_v<T, ScopeBegin>) {
                    // Scope markers are compile-time only.
                } else if constexpr (std::is_same_v<T, ScopeEnd>) {
                    if (!if_patches.empty() && if_patches.back().then_end == index) {
                        IfPatch& patch = if_patches.back();
                        if (patch.else_end == ast.nodes.size()) {
                            patch_u32(bytecode, patch.condition_target, code_offset(bytecode));
                            if_patches.pop_back();
                        } else {
                            emit_u8(bytecode, OP_JMP);
                            patch.end_target = bytecode.size();
                            emit_u32(bytecode, 0);
                            patch_u32(bytecode, patch.condition_target, code_offset(bytecode));
                        }
                    } else if (!if_patches.empty() && if_patches.back().else_end == index) {
                        patch_u32(bytecode, if_patches.back().end_target, code_offset(bytecode));
                        if_patches.pop_back();
                    }
                } else if constexpr (std::is_same_v<T, VarDecl>) {
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.runtime_index >= 0 ? value.runtime_index : value.name_index));
                } else if constexpr (std::is_same_v<T, VarChange>) {
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.runtime_index >= 0 ? value.runtime_index : value.name_index));
                } else if constexpr (std::is_same_v<T, VarRead>) {
                    emit_u8(bytecode, OP_LOAD);
                    emit_u32(bytecode, static_cast<uint32_t>(value.runtime_index >= 0 ? value.runtime_index : value.name_index));
                } else if constexpr (std::is_same_v<T, Literal>) {
                    emit_u8(bytecode, OP_PUSH);
                    emit_u32(bytecode, static_cast<uint32_t>(value.value));
                } else if constexpr (std::is_same_v<T, WriteNode>) {
                    if (value.variable_index >= 0) {
                        emit_u8(bytecode, OP_LOAD);
                        emit_u32(bytecode, static_cast<uint32_t>(value.runtime_index >= 0 ? value.runtime_index : value.variable_index));
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
                        case '=': operation = OP_EQU; break;
                        case '!': operation = OP_NEQ; break;
                    }
                    emit_u8(bytecode, operation);
                } else if constexpr (std::is_same_v<T, ReadNode>) {
                    emit_u8(bytecode, OP_READ);
                    emit_u8(bytecode, OP_STORE);
                    emit_u32(bytecode, static_cast<uint32_t>(value.runtime_index >= 0 ? value.runtime_index : value.var_idx));
                } else if constexpr (std::is_same_v<T, IfStmt>) {
                    emit_u8(bytecode, OP_JZ);
                    const size_t condition_target = bytecode.size();
                    emit_u32(bytecode, 0);
                    const NodeId no_else = static_cast<NodeId>(-1);
                    const size_t then_end = scope_end_for_begin[value.then_do];
                    const size_t else_end =
                        value.else_do == no_else ? ast.nodes.size() : scope_end_for_begin[value.else_do];
                    if_patches.push_back(IfPatch{then_end, else_end, condition_target});
                }

            },
            node.data);
    }

    emit_u8(bytecode, OP_HALT);
    return {bytecode, cpool};
}