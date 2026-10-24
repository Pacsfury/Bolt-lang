#include "../../frontend/astgen/astgen.hpp"

#include <string>
#include <cstdint>
#include <vector>
#include <variant>
#include <type_traits>
#include "codegen.hpp"

namespace {
    void emit_u8(std::vector<uint8_t>& out, uint8_t v) {
        out.push_back(v);
    }

    void emit_u32(std::vector<uint8_t>& out, uint32_t v) {
        out.push_back((v >> 0) & 0xFF);
        out.push_back((v >> 8) & 0xFF);
        out.push_back((v >> 16) & 0xFF);
        out.push_back((v >> 24) & 0xFF);
    }
}

std::vector<uint8_t> generateBytecode(const AST& ast, const std::string& path) {
    std::vector<uint8_t> bytecode;
    std::vector<uint8_t> header;
    bytecode.reserve(ast.nodes.size() * 8);
    header.reserve(ast.nodes.size() / 8);
    emit_u8(header, 'B');
    emit_u8(header, 'L');
    emit_u8(header, 'T');

    for (const auto& node : ast.nodes) {
        std::visit([&](const auto& value) {
            using T = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<T, VarDecl>) {
                
            } else if constexpr (std::is_same_v<T, Literal>) {
                emit_u8(bytecode, 0x10);
                emit_u32(bytecode, static_cast<uint32_t>(value.value));
            }
        }, node.data);
    }

    header.insert(header.end(), bytecode.begin(), bytecode.end());
    return header;
}