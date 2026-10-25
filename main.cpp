#include <string>

#include "runner/arguments.hpp"
#include "runner/runmodes.hpp"

int main(int argc, char* argv[]) {
    ArgsContext args({argc, argv});
    const static std::string OUT = std::string(args.getArg("-o").value_or("main.bvm"));

    if (args.hasArg("run")) {
        normalRun(std::string(args.getArg("run").value_or("main.blt")), OUT);
    }

    if (args.hasArg("debug")) {
        debugRun(std::string(args.getArg("debug").value_or("main.blt")), OUT);
    }

    if (args.hasArg("time")) {
        timeRun(std::string(args.getArg("time").value_or("main.blt")), OUT);
    }

    if (args.hasArg("check")) {
        checkRun(std::string(args.getArg("check").value_or("main.blt")));
    }

    if (args.hasArg("-d")) {
        run(std::string(args.getArg("-d").value_or("")), OUT);
    }

    return 0;
}
