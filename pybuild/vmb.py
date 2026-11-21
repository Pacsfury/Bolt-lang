import os
import glob
import subprocess
from dotenv import load_dotenv

def build():
    load_dotenv()
    
    compiler = os.getenv("C_COMPILER", "gcc").upper()

    # Busquem els fitxers font de C
    source_files = glob.glob("vm/*.c")
    
    if not source_files:
        print("Files not found.")
        return

    if compiler in ["MSVC", "CL"]:
        command = ["cl", "/O2"] + source_files + ["/Fe:bvm.exe"]
        
    elif compiler in ["GCC", "G++"]:
        command = ["gcc", "-std=c17", "-O3"] + source_files + ["-o", "bvm"]
        
    elif compiler in ["CLANG", "CLANG++"]:
        command = ["clang", "-std=c17", "-O3"] + source_files + ["-o", "bvm"]
        
    else:
        print(f"Unsupported compiler: {compiler}")
        return

    print(f"Compiling with {compiler}...")
    
    try:
        subprocess.run(command, check=True, text=True)
        print("BoltVM compiled successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Compilation error. Exit code: {e.returncode}")
    except FileNotFoundError:
        print(f"Error: Compiler '{command[0]}' not found in PATH.")

if __name__ == "__main__":
    build()