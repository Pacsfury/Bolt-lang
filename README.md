![Bolt Logo](assets/logo.png)

# Bolt Language

**A small, fast language and bytecode virtual machine.**

Bolt is an experimental compiled language implemented in C and C++.
It currently supports variables, constants, basic arithmetic, input/output,
and compiling source into bytecode for the Bolt Virtual Machine (BVM).

> Bolt is under active development. Syntax and command-line behavior may change.

## Features

- C++17 frontend and backend
- C17 bytecode virtual machine
- `var` and `const` variables
- `write` and `read` statements
- Integer and arithmetic expressions
- Debug and timing build modes
- Standalone executables containing the VM and compiled program
- If-else

## Requirements

- CMake 3.16 or newer for the recommended build system
- Python 3 only for the deprecated Python build scripts
- A C and C++ compiler with C17 and C++17 support
- A CMake generator supported by your platform
  (not required when using the Python scripts)

## Building

From the repository root, configure and build the compiler and VM:

```bash
cmake -S . -B cmake-build
cmake --build cmake-build --config Release
```

This produces:

- `bolt` (or `bolt.exe`) - the compiler and packager
- `bvm` (or `bvm.exe`) - the bytecode virtual machine

The exact executable location depends on the CMake generator. With the
Visual Studio generator, executables are commonly written to
`cmake-build/Release/`.

The frontend and backend libraries can be built separately:

```bash
cmake --build cmake-build --config Release --target frontend
cmake --build cmake-build --config Release --target backend
```

On Windows, a Visual Studio developer prompt is recommended. On Linux and
macOS, CMake and a compiler such as GCC or Clang are sufficient.

### Deprecated Python build system

The Python build scripts are deprecated and remain available as a fallback
for environments where CMake is not installed. The CMake build system is
recommended because it is better maintained and supports all current targets.

```bash
python -m pip install python-dotenv

python pybuild/build.py lang
python pybuild/build.py vm
python pybuild/build.py frontend
python pybuild/build.py backend
```

The Python scripts use these environment variables:

- `CPP_COMPILER`: `G++`, `CLANG`, or `MSVC`/`CL`
- `C_COMPILER`: `GCC`, `CLANG`, or `MSVC`/`CL`

## Quick Start

Create `main.blt`:

```bolt
write "Hello, world!";
```

Compile it with Bolt:

```bash
bolt build main.blt -o main.bvm
```

The build command creates two files:

- `main.bvm` - the compiled bytecode
- `BVM-CPOOL` - the constant pool used for strings and other constants

Keep `main.bvm` and `BVM-CPOOL` in the same working directory, then run:

```bash
bvm
```

> **Windows:** use `.\bolt.exe` and `.\bvm.exe` if the executables are not on
> `PATH`.

Bolt does not execute source code directly yet; build a bytecode file first,
then run it with BVM.

## Command-Line Usage

The general form is:

```text
bolt <mode> <input> -o <output>
```

If `<input>` is omitted, `main.blt` is used. If `-o` is omitted, the output
is `main.bvm`.

### Build

Compile source code to bytecode:

```bash
bolt build main.blt -o main.bvm
```

### Debug

Print tokens and the AST, then compile the source:

```bash
bolt debug main.blt -o main.bvm
```

### Time

Print timing information for each compilation stage and generate bytecode:

```bash
bolt time main.blt -o main.bvm
```

Example stages include lexing, AST generation, checking, and code generation.

### Check

Parse and check a source file without producing bytecode:

```bash
bolt check main.blt
```

This mode is intended for editors and other tooling.

### Direct

Compile a short source string without a source file:

```bash
bolt direct "var name = 'a';" -o out.bvm
```

The available modes are not currently combinable.

## Packaging

Packaging combines BVM, the constant pool, and bytecode into one executable.
This lets the program run without separate `BVM-CPOOL` and `main.bvm` files.

From the repository root, with the default files present:

```bash
bolt package
```

The default paths are:

| Input | Default Windows path | Default other-platform path |
| --- | --- | --- |
| Constant pool | `BVM-CPOOL` | `BVM-CPOOL` |
| Bytecode | `main.bvm` | `main.bvm` |
| VM executable | `vm/bvm.exe` | `vm/bvm` |
| Output executable | `program.exe` | `program` |

Specify custom paths with:

```bash
bolt package -C BVM-CPOOL -B main.bvm -vm vm/bvm.exe -o BoltProgram.exe
```

- `-C` selects the constant pool.
- `-B` selects the bytecode file.
- `-vm` selects the BVM executable.
- `-o` selects the packaged executable.

The output executable must not overwrite any input file.

We recommend just using this:
```bash
bolt package -vm VM-PATH -o output
```

## Language Syntax

### Output

```bolt
write "A string";
write 'C';
```

`write` prints a value to standard output.

### Variables

Variables can be declared with `var`:

```bolt
var name = 'a';
var greeting = "Hello";
var number = 123;
greeting = "Welcome";
```

Constants can be declared with `const`:

```bolt
const base = 100;
```

Explicit type annotations are recognized in some syntax experiments, but they
are not fully supported yet.

```bolt
var points: i16 = 45;
```

The safest current form is to declare variables without an explicit type.

### Input

```bolt
var status = "";

write "How is the work going?";
read status;

write "The status is: ";
write status;
```

`read` reads input into an existing variable.

### Arithmetic

```bolt
var result = 10 + 20 * 30 - 40 / 2;
```

Bolt currently supports `+`, `-`, `*`, and `/`, and `==`, `!=`.

### Functions

Function syntax is experimental and may change:

```bolt
fn add(a: i32, b: i32) -> i32 {
    ret a + b;
}

var answer = add(10, 20);
```

## If-else

```bolt
if cond {
  ---
} else {
  ---
}
```

Bolt currently supports scopes.

## Examples

Example programs are in [`examples/`](examples/):

- [`examples/io.blt`](examples/io.blt) - input and output
- [`examples/math.blt`](examples/math.blt) - arithmetic
- [`examples/equality.blt`](examples/equality.blt) - equality operators (== and !=)
- [`examples/scopes.blt`](examples/scopes.blt) - scopes
- [`examples/if-else.blt`](examples/if-else.blt) - control flow
- [`examples/questions.blt`](examples/questions.blt) - io and variables

More examples as new features keep coming.

## Benchmarks

See [`BENCH.md`](BENCH.md) for compilation and VM benchmark results.

## Project Structure

| Path | Purpose |
| --- | --- |
| `frontend/` | Lexer, AST generator, and checkers |
| `backend/` | Bytecode generation |
| `runner/` | Command-line modes |
| `packager/` | Standalone executable packaging |
| `vm/` | Bolt Virtual Machine implementation |
| `debug/` | Debug and diagnostic output |
| `common/` | Shared bytecode definitions |
| `examples/` | Example Bolt programs |

## License

Bolt is available under the [MIT License](LICENSE).
