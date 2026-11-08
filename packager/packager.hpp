#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
bool create_package_executable(const std::string& cpool_path = "BVM-CPOOL",
                               const std::string& bytecode_path = "main.bvm",
                               const std::string& vm_path = "vm/vm.exe",
                               const std::string& output_exe_path = "OUTPUT.exe");

#else
bool create_package_executable(const std::string& cpool_path = "BVM-CPOOL",
                               const std::string& bytecode_path = "main.bvm",
                               const std::string& vm_path = "vm/vm",
                               const std::string& output_exe_path = "OUTPUT");
#endif
