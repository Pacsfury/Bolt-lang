#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <iostream>

#include "../backend/codegen/codegen.hpp"
#include "../frontend/astgen/astgen.hpp"
#include "../frontend/checker/structure.hpp"
#include "../frontend/lexer/tokens.hpp"
#include "../debug/errors.hpp"
#include "../debug/print.hpp"

void normalRun(std::string file, std::string out) {
    std::ifstream input(file);
    if (!input.is_open()) {
        throwError("Bolt File Error: Could't open '" + file + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();

    const AST ast = createAST(tokenize(content));

    checkAST(ast);
    const auto bytecode = generateBytecode(ast, out);

    std::ofstream output(out, std::ios::binary);
    if (!output) {
        throwError("Bolt File Error: Couldn't open file '"+out+"'");
    }

    output.write(reinterpret_cast<const char*>(bytecode.data()), static_cast<std::streamsize>(bytecode.size()));
    output.close();
}

void debugRun(std::string file, std::string out) {
    std::ifstream input(file);
    if (!input.is_open()) {
        throwError("Bolt File Error: Could't open '" + file + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();

    const std::vector<Token> tokens = tokenize(content);
    show(tokens);

    const AST ast = createAST(tokens);
    show(ast);

    checkAST(ast);
    const auto bytecode = generateBytecode(ast, out);

    std::ofstream output(out, std::ios::binary);
    if (!output) {
        throwError("Bolt File Error: Couldn't open file '"+out+"'");
    }

    output.write(reinterpret_cast<const char*>(bytecode.data()), static_cast<std::streamsize>(bytecode.size()));
    output.close();
}

void timeRun(std::string file, std::string out) {
    std::ifstream input(file);
    if (!input.is_open()) {
        throwError("Bolt File Error: Could't open '" + file + "'");
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    std::string content = buffer.str();

    auto lexstart = std::chrono::high_resolution_clock::now();

    auto tokens = tokenize(content);

    auto lexend = std::chrono::high_resolution_clock::now();

    show(tokens);

    auto lextime = std::chrono::duration_cast<std::chrono::nanoseconds>(lexend - lexstart).count();

    std::cout << "Lexing time: " << lextime << " nanosecons." << std::endl;
    std::cout << "Or: " << lextime / 1000000.0 << " ms." << std::endl;

    auto astart = std::chrono::high_resolution_clock::now();
    auto ast = createAST(tokens);

    auto astend = std::chrono::high_resolution_clock::now();
    auto asttime = std::chrono::duration_cast<std::chrono::nanoseconds>(astend - astart).count();

    std::cout << "ASTgen time: " << asttime << " nanosecons." << std::endl;
    std::cout << "Or: " << asttime / 1000000.0 << " ms." << std::endl;

    show(ast);

    auto checkstart = std::chrono::high_resolution_clock::now();
    checkAST(ast);

    auto checkend = std::chrono::high_resolution_clock::now();
    auto checktime = std::chrono::duration_cast<std::chrono::nanoseconds>(checkend - checkstart).count();

    std::cout << "Checking time: " << checktime << " nanosecons." << std::endl;
    std::cout << "Or: " << checktime / 1000000.0 << " ms." << std::endl;

    auto genstart = std::chrono::high_resolution_clock::now();
    auto bytecode = generateBytecode(ast);

    auto genend = std::chrono::high_resolution_clock::now();
    auto gentime = std::chrono::duration_cast<std::chrono::nanoseconds>(genend - genstart).count();

    std::cout << "Codegen time: " << gentime << " nanosecons." << std::endl;
    std::cout << "Or: " << gentime / 1000000.0 << " ms." << std::endl;

    std::ofstream output(out, std::ios::binary);
    if (!output) {
        throwError("Bolt File Error: Couldn't open file '"+out+"'");
    }

    output.write(reinterpret_cast<const char*>(bytecode.data()), static_cast<std::streamsize>(bytecode.size()));
    output.close();
}

