#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#undef max
#undef min
#endif

#include "packager.hpp"

namespace fs = std::filesystem;

namespace {

constexpr std::array<char, 8> kSfxMagic = {'B', 'O', 'L', 'T', 'S', 'F', 'X', '1'};

fs::path current_executable_path() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(1024);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            throw std::runtime_error("Could not determine the current executable path.");
        }
        if (length < buffer.size() - 1) {
            return fs::path(std::wstring(buffer.data(), length));
        }
        if (buffer.size() >= 32768) {
            throw std::runtime_error("The current executable path is too long.");
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    throw std::runtime_error("Self-extracting executables are supported on Windows only.");
#endif
}

void write_u64(std::ostream& stream, std::uint64_t value) {
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        stream.put(static_cast<char>((value >> shift) & 0xFF));
    }
}

std::uint64_t read_u64(std::istream& stream) {
    std::uint64_t value = 0;
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        const int byte = stream.get();
        if (byte == std::char_traits<char>::eof()) {
            throw std::runtime_error("Invalid self-extracting package footer.");
        }
        value |= static_cast<std::uint64_t>(static_cast<unsigned char>(byte)) << shift;
    }
    return value;
}

}  // namespace

// Helper to read any binary file into memory
std::vector<char> read_binary(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open: " + filepath);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Could not read: " + filepath);
    }
    return buffer;
}

// Simple CRC-32 checksum calculation required for ZIP headers
uint32_t compute_crc32(const char* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint8_t>(data[i]);
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (-(crc & 1) & 0xEDB88320);
        }
    }
    return ~crc;
}

struct ZipEntry {
    std::string filename;
    std::vector<char> data;
    uint32_t crc32;
};

// Generates the deployable ZIP package using pure C++ file writing
bool create_package_zip(const std::string& cpool_path, const std::string& bytecode_path, const std::string& vm_path,
                        const std::string& output_zip_path) {
    try {
        // 1. Read the files into memory
        std::vector<char> cpool_data = read_binary(cpool_path);
        std::vector<char> bytecode_data = read_binary(bytecode_path);
        std::vector<char> vm_data = read_binary(vm_path);

        std::vector<ZipEntry> entries = {
            {"CONSTANTPOOL", cpool_data, compute_crc32(cpool_data.data(), cpool_data.size())},
            {"bytecode", bytecode_data, compute_crc32(bytecode_data.data(), bytecode_data.size())},
            {"vm.exe", vm_data, compute_crc32(vm_data.data(), vm_data.size())}};

        // 2. Open output zip file stream
        std::ofstream zip(output_zip_path, std::ios::binary);
        if (!zip.is_open()) {
            std::cerr << "Failed to create zip: " << output_zip_path << std::endl;
            return false;
        }

        struct CentralDirRecord {
            std::string name;
            uint32_t crc32;
            uint32_t size;
            uint32_t local_header_offset;
        };
        std::vector<CentralDirRecord> cd_records;
        uint32_t current_offset = 0;

        // 3. Write Local File Headers and Data (STORE / Uncompressed method)
        for (const auto& entry : entries) {
            uint32_t local_offset = current_offset;
            uint32_t size = static_cast<uint32_t>(entry.data.size());
            uint16_t name_len = static_cast<uint16_t>(entry.filename.size());

            // Local File Header Signature (PK\03\04)
            zip.write("\x50\x4B\x03\x04", 4);
            zip.write("\x14\x00", 2);          // Version needed
            zip.write("\x00\x00", 2);          // Flags
            zip.write("\x00\x00", 2);          // Compression (0 = Stored)
            zip.write("\x00\x00\x00\x00", 4);  // Time/Date
            zip.write(reinterpret_cast<const char*>(&entry.crc32), 4);
            zip.write(reinterpret_cast<const char*>(&size), 4);  // Compressed size
            zip.write(reinterpret_cast<const char*>(&size), 4);  // Uncompressed size
            zip.write(reinterpret_cast<const char*>(&name_len), 2);
            zip.write("\x00\x00", 2);  // Extra field length

            // Filename and Payload Data
            zip.write(entry.filename.data(), name_len);
            zip.write(entry.data.data(), size);

            cd_records.push_back({entry.filename, entry.crc32, size, local_offset});
            current_offset += (30 + name_len + size);
        }

        // 4. Write Central Directory
        uint32_t cd_start_offset = current_offset;
        uint32_t cd_size = 0;

        for (const auto& cd : cd_records) {
            uint16_t name_len = static_cast<uint16_t>(cd.name.size());

            // Central Directory Header Signature (PK\01\02)
            zip.write("\x50\x4B\x01\x02", 4);
            zip.write("\x14\x00", 2);          // Version made by
            zip.write("\x14\x00", 2);          // Version needed
            zip.write("\x00\x00", 2);          // Flags
            zip.write("\x00\x00", 2);          // Compression
            zip.write("\x00\x00\x00\x00", 4);  // Time/Date
            zip.write(reinterpret_cast<const char*>(&cd.crc32), 4);
            zip.write(reinterpret_cast<const char*>(&cd.size), 4);
            zip.write(reinterpret_cast<const char*>(&cd.size), 4);
            zip.write(reinterpret_cast<const char*>(&name_len), 2);
            zip.write("\x00\x00", 2);  // Extra len
            zip.write("\x00\x00", 2);  // Comment len
            zip.write("\x00\x00", 2);  // Disk start
            zip.write("\x00\x00", 2);  // Internal attr

            // External attributes (mark vm.exe as executable if desired)
            uint32_t ext_attr = (cd.name == "vm.exe") ? 0x81ED0000 : 0x00000000;
            zip.write(reinterpret_cast<const char*>(&ext_attr), 4);
            zip.write(reinterpret_cast<const char*>(&cd.local_header_offset), 4);

            zip.write(cd.name.data(), name_len);
            cd_size += (46 + name_len);
        }

        // 5. Write End of Central Directory Record (EOCD) (PK\05\06)
        zip.write("\x50\x4B\x05\x06", 4);
        zip.write("\x00\x00", 2);  // Disk number
        zip.write("\x00\x00", 2);  // Central dir disk
        uint16_t total_records = static_cast<uint16_t>(cd_records.size());
        zip.write(reinterpret_cast<const char*>(&total_records), 2);
        zip.write(reinterpret_cast<const char*>(&total_records), 2);
        zip.write(reinterpret_cast<const char*>(&cd_size), 4);
        zip.write(reinterpret_cast<const char*>(&cd_start_offset), 4);
        zip.write("\x00\x00", 2);  // Comment length

        zip.close();
        std::cout << "Package generated successfully: " << output_zip_path << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

namespace {

bool create_package_executable_impl(const std::string& cpool_path, const std::string& bytecode_path,
                                    const std::string& vm_path, const std::string& output_exe_path,
                                    const fs::path& builder_path) {
    const fs::path output_path = fs::absolute(output_exe_path).lexically_normal();
    if (output_path == fs::absolute(builder_path).lexically_normal()) {
        std::cerr << "Output executable must not overwrite the package builder.\n";
        return false;
    }

    fs::path build_dir;
    std::error_code error;
    const fs::path temp_root = fs::temp_directory_path(error);
    if (error) {
        std::cerr << "Could not locate the temporary directory: " << error.message() << '\n';
        return false;
    }

#ifdef _WIN32
    const unsigned long process_id = GetCurrentProcessId();
#else
    const unsigned long process_id = 0;
#endif
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        build_dir = temp_root / ("BoltPackageBuild-" + std::to_string(process_id) + "-" + std::to_string(attempt));
        if (fs::create_directory(build_dir, error)) {
            break;
        }
        if (error && error != std::errc::file_exists) {
            std::cerr << "Could not create temporary build directory: " << error.message() << '\n';
            return false;
        }
        error.clear();
        build_dir.clear();
    }
    if (build_dir.empty()) {
        std::cerr << "Could not reserve a temporary build directory.\n";
        return false;
    }

    const fs::path zip_path = build_dir / "package.zip";
    if (!create_package_zip(cpool_path, bytecode_path, vm_path, zip_path.string())) {
        fs::remove_all(build_dir, error);
        return false;
    }

    fs::copy_file(builder_path, output_path, fs::copy_options::overwrite_existing, error);
    if (error) {
        std::cerr << "Could not create output executable: " << error.message() << '\n';
        fs::remove_all(build_dir, error);
        return false;
    }

    std::ifstream zip(zip_path, std::ios::binary);
    std::ofstream output(output_path, std::ios::binary | std::ios::app);
    if (!zip.is_open() || !output.is_open()) {
        std::cerr << "Could not open package files for writing.\n";
        fs::remove(output_path, error);
        fs::remove_all(build_dir, error);
        return false;
    }

    zip.seekg(0, std::ios::end);
    const std::streamoff zip_size = zip.tellg();
    if (zip_size < 0) {
        std::cerr << "Could not determine package size.\n";
        fs::remove(output_path, error);
        fs::remove_all(build_dir, error);
        return false;
    }
    zip.seekg(0, std::ios::beg);
    std::array<char, 64 * 1024> buffer{};
    while (zip) {
        zip.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = zip.gcount();
        if (count > 0) {
            output.write(buffer.data(), count);
        }
    }
    if (!zip.eof() || !output) {
        std::cerr << "Could not append package data to the executable.\n";
        zip.close();
        output.close();
        fs::remove(output_path, error);
        fs::remove_all(build_dir, error);
        return false;
    }

    output.write(kSfxMagic.data(), static_cast<std::streamsize>(kSfxMagic.size()));
    write_u64(output, static_cast<std::uint64_t>(zip_size));
    output.close();
    zip.close();
    if (!output) {
        std::cerr << "Could not finish writing output executable.\n";
        fs::remove(output_path, error);
        fs::remove_all(build_dir, error);
        return false;
    }

    fs::remove_all(build_dir, error);
    if (error) {
        std::cerr << "Warning: could not remove temporary build directory: " << error.message() << '\n';
    }
    std::cout << "Self-extracting executable created: " << output_path.string() << '\n';
    return true;
}

std::uint16_t read_u16(const std::vector<char>& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 2) {
        throw std::runtime_error("Invalid ZIP header.");
    }
    return static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset]) |
                                      (static_cast<std::uint16_t>(static_cast<unsigned char>(data[offset + 1])) << 8));
}

std::uint32_t read_u32(const std::vector<char>& data, std::size_t offset) {
    if (offset > data.size() || data.size() - offset < 4) {
        throw std::runtime_error("Invalid ZIP header.");
    }
    return static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(data[offset + 3])) << 24);
}

std::vector<char> read_appended_zip(const fs::path& executable_path) {
    std::ifstream file(executable_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open the self-extracting executable.");
    }

    const std::streamoff file_size = file.tellg();
    constexpr std::streamoff footer_size = 16;
    if (file_size < footer_size) {
        return {};
    }
    file.seekg(file_size - footer_size, std::ios::beg);
    std::array<char, 8> magic{};
    if (!file.read(magic.data(), static_cast<std::streamsize>(magic.size())) || magic != kSfxMagic) {
        return {};
    }
    const std::uint64_t zip_size = read_u64(file);
    if (zip_size > static_cast<std::uint64_t>(file_size - footer_size) ||
        zip_size > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("Invalid embedded ZIP size.");
    }

    std::vector<char> zip(static_cast<std::size_t>(zip_size));
    file.seekg(file_size - footer_size - static_cast<std::streamoff>(zip_size), std::ios::beg);
    if (!zip.empty() && !file.read(zip.data(), static_cast<std::streamsize>(zip.size()))) {
        throw std::runtime_error("Could not read the embedded ZIP.");
    }
    return zip;
}

void extract_package_zip(const std::vector<char>& zip, const fs::path& destination) {
    if (zip.size() < 22 || read_u32(zip, zip.size() - 22) != 0x06054B50) {
        throw std::runtime_error("The embedded ZIP has no valid end record.");
    }

    const std::uint16_t entry_count = read_u16(zip, zip.size() - 22 + 10);
    const std::uint32_t central_size = read_u32(zip, zip.size() - 22 + 12);
    const std::uint32_t central_offset = read_u32(zip, zip.size() - 22 + 16);
    if (static_cast<std::uint64_t>(central_offset) + central_size > zip.size() - 22) {
        throw std::runtime_error("The embedded ZIP directory is invalid.");
    }

    std::vector<std::string> extracted;
    std::size_t record = central_offset;
    for (std::uint16_t i = 0; i < entry_count; ++i) {
        if (read_u32(zip, record) != 0x02014B50) {
            throw std::runtime_error("The embedded ZIP directory entry is invalid.");
        }
        const std::uint16_t method = read_u16(zip, record + 10);
        const std::uint32_t expected_crc = read_u32(zip, record + 16);
        const std::uint32_t compressed_size = read_u32(zip, record + 20);
        const std::uint32_t uncompressed_size = read_u32(zip, record + 24);
        const std::uint16_t name_length = read_u16(zip, record + 28);
        const std::uint16_t extra_length = read_u16(zip, record + 30);
        const std::uint16_t comment_length = read_u16(zip, record + 32);
        const std::uint32_t local_offset = read_u32(zip, record + 42);
        const std::size_t variable_size = static_cast<std::size_t>(name_length) + extra_length + comment_length;
        if (record > zip.size() || zip.size() - record < 46 || zip.size() - record - 46 < variable_size) {
            throw std::runtime_error("The embedded ZIP directory entry is truncated.");
        }

        const std::string name(zip.data() + record + 46, name_length);
        std::string output_name;
        if (name == "CONSTANTPOOL") {
            output_name = "BVM-CPOOL";
        } else if (name == "bytecode") {
            output_name = "main.bvm";
        } else if (name == "vm.exe") {
            output_name = "vm.exe";
        } else {
            throw std::runtime_error("Unexpected file in package: " + name);
        }
        if (std::find(extracted.begin(), extracted.end(), output_name) != extracted.end()) {
            throw std::runtime_error("Duplicate file in package: " + output_name);
        }

        if (method != 0 || compressed_size != uncompressed_size || read_u32(zip, local_offset) != 0x04034B50) {
            throw std::runtime_error("Unsupported or invalid ZIP entry: " + name);
        }
        const std::uint16_t local_name_length = read_u16(zip, local_offset + 26);
        const std::uint16_t local_extra_length = read_u16(zip, local_offset + 28);
        const std::uint64_t data_offset =
            static_cast<std::uint64_t>(local_offset) + 30 + local_name_length + local_extra_length;
        if (data_offset > zip.size() || compressed_size > zip.size() - data_offset) {
            throw std::runtime_error("ZIP entry data is truncated: " + name);
        }

        const char* data = zip.data() + static_cast<std::size_t>(data_offset);
        if (compute_crc32(data, uncompressed_size) != expected_crc) {
            throw std::runtime_error("ZIP entry checksum failed: " + name);
        }
        std::ofstream output(destination / output_name, std::ios::binary);
        if (!output.is_open() ||
            (uncompressed_size > 0 && !output.write(data, static_cast<std::streamsize>(uncompressed_size)))) {
            throw std::runtime_error("Could not extract: " + output_name);
        }
        extracted.push_back(output_name);
        record += 46 + variable_size;
    }

    if (extracted.size() != 3 || std::find(extracted.begin(), extracted.end(), "BVM-CPOOL") == extracted.end() ||
        std::find(extracted.begin(), extracted.end(), "main.bvm") == extracted.end() ||
        std::find(extracted.begin(), extracted.end(), "vm.exe") == extracted.end()) {
        throw std::runtime_error("Package must contain VM, bytecode, and constant pool.");
    }
}

int run_embedded_package_impl(const fs::path& executable_path) {
    const std::vector<char> zip = read_appended_zip(executable_path);
    if (zip.empty()) {
        return -1;
    }

#ifdef _WIN32
    std::error_code error;
    const fs::path temp_root = fs::temp_directory_path(error);
    if (error) {
        throw std::runtime_error("Could not locate the temporary directory: " + error.message());
    }

    fs::path extraction_dir;
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        extraction_dir =
            temp_root / ("BoltPackage-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(attempt));
        if (fs::create_directory(extraction_dir, error)) {
            break;
        }
        if (error && error != std::errc::file_exists) {
            throw std::runtime_error("Could not create extraction directory: " + error.message());
        }
        error.clear();
        extraction_dir.clear();
    }
    if (extraction_dir.empty()) {
        throw std::runtime_error("Could not reserve an extraction directory.");
    }

    struct ExtractionCleanup {
        const fs::path& path;

        ~ExtractionCleanup() {
            std::error_code cleanup_error;
            fs::remove_all(path, cleanup_error);
            if (cleanup_error) {
                std::cerr << "Warning: could not remove temporary package folder "
                          << path.string() << ": " << cleanup_error.message() << '\n';
            }
        }
    } cleanup{extraction_dir};

    extract_package_zip(zip, extraction_dir);
    const fs::path vm_path = extraction_dir / L"vm.exe";
    std::wstring command_line = L"\"" + vm_path.wstring() + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(vm_path.c_str(), command_line.data(), nullptr, nullptr, FALSE, 0, nullptr,
                        extraction_dir.c_str(), &startup, &process)) {
        throw std::runtime_error("Could not start vm.exe. Windows error: " + std::to_string(GetLastError()));
    }
    CloseHandle(process.hThread);
    if (WaitForSingleObject(process.hProcess, INFINITE) != WAIT_OBJECT_0) {
        const DWORD error_code = GetLastError();
        CloseHandle(process.hProcess);
        throw std::runtime_error("Could not wait for VM to finish. Windows error: " +
                                 std::to_string(error_code));
    }
    DWORD exit_code = 1;
    if (!GetExitCodeProcess(process.hProcess, &exit_code)) {
        const DWORD error_code = GetLastError();
        CloseHandle(process.hProcess);
        throw std::runtime_error("Could not read VM exit code. Windows error: " + std::to_string(error_code));
    }
    CloseHandle(process.hProcess);
    return static_cast<int>(exit_code);
#else
    throw std::runtime_error("Self-extracting executables are supported on Windows only.");
#endif
}

}  // namespace

#ifndef BOLT_LIBRARY_BUILD
int main(int argc, char* argv[]) {
    try {
        const fs::path executable_path = current_executable_path();
        const int package_exit_code = run_embedded_package_impl(executable_path);
        if (package_exit_code >= 0) {
            return package_exit_code;
        }

        if (argc != 1 && argc != 5) {
            std::cerr << "Usage: bolt-package.exe [CPOOL_FILE BYTECODE_FILE VM_EXE OUTPUT_EXE]\n";
            return 2;
        }
        const std::string cpool_path = argc == 5 ? argv[1] : "BVM-CPOOL";
        const std::string bytecode_path = argc == 5 ? argv[2] : "main.bvm";
        const std::string vm_path = argc == 5 ? argv[3] : "vm/bvm.exe";
        const std::string output_path = argc == 5 ? argv[4] : "BoltProgram.exe";
        return create_package_executable_impl(cpool_path, bytecode_path, vm_path,
                                              output_path, executable_path)
                   ? 0
                   : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
#endif

bool create_package_executable(const std::string& cpool_path,
                               const std::string& bytecode_path,
                               const std::string& vm_path,
                               const std::string& output_exe_path,
                               const std::filesystem::path& builder_path) {
    return create_package_executable_impl(cpool_path, bytecode_path, vm_path,
                                          output_exe_path, builder_path);
}

int run_embedded_package(const std::filesystem::path& executable_path) {
    return run_embedded_package_impl(executable_path);
}
