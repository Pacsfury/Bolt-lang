#pragma once
#include <string>

void resetVariables();

void registervar(std::string name, bool isconst);

void checkVariableRead(const std::string& name);

void checkAssignment(const std::string& name);