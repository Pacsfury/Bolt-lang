#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

#include "packager.hpp"

namespace fs = std::filesystem;

constexpr std::array<char, 8> kAppendMagic = {'B', 'O', 'L', 'T', 'P', 'K', 'G', '1'};

void write_u64(std::ostream& stream, std::uint64_t value) {
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        stream.put(static_cast<char>((value >> shift) & 0xFF));
    }
}

bool create_package_executable(const std::string& cpool_path, const std::string& bytecode_path,
                               const std::string& vm_path, const std::string& output_exe_path) {
    const fs::path output_path = fs::absolute(output_exe_path).lexically_normal();
    const fs::path cpool_input = fs::absolute(cpool_path).lexically_normal();
    const fs::path bytecode_input = fs::absolute(bytecode_path).lexically_normal();
    const fs::path vm_input = fs::absolute(vm_path).lexically_normal();
    if (output_path == cpool_input || output_path == bytecode_input || output_path == vm_input) {
        std::cerr << "Output executable must not overwrite an input file.\n";
        return false;
    }

    std::error_code error;
    const std::uintmax_t vm_size = fs::file_size(vm_input, error);
    if (error) {
        std::cerr << "Could not determine VM size: " << error.message() << '\n';
        return false;
    }
    const std::uintmax_t cpool_size = fs::file_size(cpool_input, error);
    if (error) {
        std::cerr << "Could not determine CPOOL size: " << error.message() << '\n';
        return false;
    }
    const std::uintmax_t bytecode_size = fs::file_size(bytecode_input, error);
    if (error) {
        std::cerr << "Could not determine bytecode size: " << error.message() << '\n';
        return false;
    }

    constexpr std::uintmax_t max_u64 = std::numeric_limits<std::uint64_t>::max();
    if (vm_size > max_u64 || cpool_size > max_u64 || bytecode_size > max_u64 || vm_size > max_u64 - cpool_size ||
        vm_size + cpool_size > max_u64 - bytecode_size) {
        std::cerr << "Package data is too large.\n";
        return false;
    }

    fs::copy_file(vm_input, output_path, fs::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "Could not copy VM to output executable: " << error.message() << '\n';
        return false;
    }

    std::ifstream cpool(cpool_input, std::ios::binary);
    std::ifstream bytecode(bytecode_input, std::ios::binary);
    std::ofstream output(output_path, std::ios::binary | std::ios::app);
    if (!cpool.is_open() || !bytecode.is_open() || !output.is_open()) {
        std::cerr << "Could not open package files for appending.\n";
        return false;
    }

    const auto append_file = [&output](std::ifstream& input, std::uint64_t expected_size) {
        std::array<char, 64 * 1024> buffer{};
        std::uint64_t total = 0;
        while (input) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count = input.gcount();
            if (count > 0) {
                if (static_cast<std::uint64_t>(count) > expected_size - total) {
                    return false;
                }
                output.write(buffer.data(), count);
                total += static_cast<std::uint64_t>(count);
            }
        }
        return input.eof() && !input.bad() && total == expected_size && static_cast<bool>(output);
    };

    if (!append_file(cpool, static_cast<std::uint64_t>(cpool_size)) ||
        !append_file(bytecode, static_cast<std::uint64_t>(bytecode_size))) {
        std::cerr << "Could not append CPOOL or bytecode data.\n";
        return false;
    }

    const std::uint64_t cpool_offset = static_cast<std::uint64_t>(vm_size);
    const std::uint64_t bytecode_offset = cpool_offset + static_cast<std::uint64_t>(cpool_size);
    output.write(kAppendMagic.data(), static_cast<std::streamsize>(kAppendMagic.size()));
    write_u64(output, cpool_offset);
    write_u64(output, static_cast<std::uint64_t>(cpool_size));
    write_u64(output, bytecode_offset);
    write_u64(output, static_cast<std::uint64_t>(bytecode_size));
    output.close();
    if (!output) {
        std::cerr << "Could not finish writing output executable.\n";
        return false;
    }

    std::cout << "Append executable created: " << output_path.string() << '\n';
    return true;
}
