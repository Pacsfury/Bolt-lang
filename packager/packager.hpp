#pragma once

#include <filesystem>
#include <string>

bool create_package_executable(const std::string& cpool_path,
                               const std::string& bytecode_path,
                               const std::string& vm_path,
                               const std::string& output_exe_path,
                               const std::filesystem::path& builder_path);

int run_embedded_package(const std::filesystem::path& executable_path);
