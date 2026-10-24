#pragma once

#include "../../frontend/astgen/astgen.hpp"


#include <string>
#include <cstdint>
#include <vector>

std::vector<uint8_t> generateBytecode(const AST& ast, const std::string& path = "main.bvm");
