import os
import glob
import subprocess
from dotenv import load_dotenv

def build():
    load_dotenv()
    
    compiler = os.getenv("CPP_COMPILER", "g++").upper()

    source_files = glob.glob("debug/*.cpp") + glob.glob("frontend/*/*.cpp") + glob.glob("runner/*.cpp") + ["main.cpp"]
    
    if not source_files:
        print("Files not found.")
        return

    if compiler in ["MSVC", "CL"]:
        command = ["cl", "/EHsc", "/std:c++17", "/O2"] + source_files + ["/Fe:bolt.exe"]
        
    elif compiler in ["G++", "GCC"]:
        command = ["g++", "-std=c++17", "-O3"] + source_files + ["-o", "bolt"]
        
    elif compiler in ["CLANG", "CLANG++"]:
        command = ["clang++", "-std=c++17", "-O3"] + source_files + ["-o", "bolt"]
        
    else:
        print(f"Unsupported compiler: {compiler}")
        return

    print(f"Compiling with {compiler}...")
    
    try:
        result = subprocess.run(command, check=True, text=True)
        print("Bolt compiled")
    except subprocess.CalledProcessError as e:
        print(f"Error compilating: {e.returncode}")
    except FileNotFoundError:
        print(f"Error '{command[0]}'.")
