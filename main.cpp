#include <iostream>
#include <chrono>
#include <fstream>
#include <sstream>

#include "debug/print.hpp"
#include "frontend/lexer/tokens.hpp"
#include "frontend/astgen/astgen.hpp"
#include "backend/codegen/codegen.hpp"
#include "frontend/checker/structure.hpp"

int main() {
    std::ifstream input("bench.blt");
    if (!input.is_open()) {
        std::cerr<<"Bolt File Error: could't open 'bench.blt";
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

    std::ofstream output("main.bvm", std::ios::binary);
    if (!output) {
        return 1;
    }
    
    output.write(reinterpret_cast<const char*>(bytecode.data()),
                 static_cast<std::streamsize>(bytecode.size()));
    output.close();




    return 0;
}
