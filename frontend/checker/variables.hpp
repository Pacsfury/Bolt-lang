#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class VarState : uint8_t { Undeclared, Mutable, Constant };

void registervar(std::vector<VarState>& symbol_table, std::size_t index, const std::string& name, bool isconst);

void checkVariableRead(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name);

void checkAssignment(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name);