#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

enum class VarState : uint8_t { Undeclared, Mutable, Constant };

using ScopeTable = std::vector<std::unordered_map<std::size_t, VarState>>;

void enterScope(ScopeTable& scopes);
void exitScope(ScopeTable& scopes);

void registervar(std::vector<VarState>& symbol_table, std::size_t index, const std::string& name, bool isconst);
void checkVariableRead(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name);
void checkAssignment(const std::vector<VarState>& symbol_table, std::size_t index, const std::string& name);

void registervar(ScopeTable& scopes, std::size_t index, const std::string& name, bool isconst);
void checkVariableRead(const ScopeTable& scopes, std::size_t index, const std::string& name);
void checkAssignment(const ScopeTable& scopes, std::size_t index, const std::string& name);