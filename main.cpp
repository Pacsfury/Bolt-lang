#include <string>

#include "runner/arguments.hpp"
#include "runner/runmodes.hpp"


int main(int argc, char* argv[]) {

    ArgsContext args({argc, argv});

    if (args.hasArg("run")) {
        normalRun(std::string(args.getArg("run").value_or("main.blt")),
              std::string(args.getArg("-o").value_or("main.bvm")));
    }

    if (args.hasArg("debug")) {
        debugRun(std::string(args.getArg("debug").value_or("main.blt")),
              std::string(args.getArg("-o").value_or("main.bvm")));
    }

    if (args.hasArg("time")) {
        timeRun(std::string(args.getArg("time").value_or("main.blt")),
              std::string(args.getArg("-o").value_or("main.bvm")));
    }


    return 0;
}
