#include <string>

#include "runner/arguments.hpp"
#include "runner/runmodes.hpp"
#include "packager/packager.hpp"

int main(int argc, char* argv[]) {
    const int package_exit_code = run_embedded_package(argv[0]);
    if (package_exit_code >= 0) {
        return package_exit_code;
    }

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
        create_package_executable(std::string(args.getArg("-C").value_or("BVM-CPOOL")), std::string(args.getArg("-B").value_or("main.bvm")), std::string(args.getArg("-vm").value_or("vm/vm")), std::string(args.getArg("-o").value_or("out/program")), argv[0]);
    }

    return 0;
}
