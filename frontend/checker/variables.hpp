#pragma once
#include <string>

void resetVariables();

void registervar(std::string name, bool isconst, bool is_integer);

bool checkVariableRead(const std::string& name);

void checkAssignment(const std::string& name, bool is_integer);