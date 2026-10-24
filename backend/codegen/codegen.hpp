#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../../frontend/astgen/astgen.hpp"

std::vector<uint8_t> generateBytecode(const AST& ast, const std::string& path = "main.bvm");
