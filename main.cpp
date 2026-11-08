#include <string>

#include "runner/arguments.hpp"
#include "runner/runmodes.hpp"
#include "packager/packager.hpp"

int main(int argc, char* argv[]) {
    ArgsContext args({argc, argv});
    const static std::string OUTPUT = std::string(args.getArg("-o").value_or("main.bvm"));

    if (args.hasArg("build")) {
        normalRun(std::string(args.getArg("build").value_or("main.blt")), OUTPUT);
    }

    if (args.hasArg("debug")) {
        debugRun(std::string(args.getArg("debug").value_or("main.blt")), OUTPUT);
    }

    if (args.hasArg("time")) {
        timeRun(std::string(args.getArg("time").value_or("main.blt")), OUTPUT);
    }

    if (args.hasArg("check")) {
        checkRun(std::string(args.getArg("check").value_or("main.blt")));
    }

    if (args.hasArg("-d")) {
        run(std::string(args.getArg("-d").value_or("")), OUTPUT);
    }

    if (args.hasArg("package")) {
        #ifdef _WIN32
            create_package_executable(std::string(args.getArg("-C").value_or("BVM-CPOOL")), std::string(args.getArg("-B").value_or("main.bvm")), std::string(args.getArg("-vm").value_or("vm/bvm.exe")), std::string(args.getArg("-o").value_or("program.exe")));
        #else
            create_package_executable(std::string(args.getArg("-C").value_or("BVM-CPOOL")), std::string(args.getArg("-B").value_or("main.bvm")), std::string(args.getArg("-vm").value_or("vm/bvm")), std::string(args.getArg("-o").value_or("program")));
        #endif
    }

    return 0;
}
