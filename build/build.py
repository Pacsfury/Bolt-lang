import sys
import frontendb as front
import vmb as vm
import backendb as backend
import lang

print("\033[96mThis build system has been deprecated. Consider using the Cmake build system.\033[0m\n")

if sys.argv[1] == "frontend":
    front.build()
elif sys.argv[1] == "vm":
    vm.build()
elif sys.argv[1] == "backend":
    backend.build()
elif sys.argv[1] == "lang":
    lang.build()