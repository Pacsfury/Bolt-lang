![Bolt Logo](assets/logo.png)

# Bolt Language

A language made to be fast and simple.

---

## Building with Python

1. Set up an `.env` file

```env
CPP_COMPILER="your-cpp-compiler"
C_COMPILER="c_compiler"
```

2. Run Python to compile

> There is no backend yet, so you can't compile backend

```bash
python build/build.py frontend
python build/build.py vm
```

## Run modes

Currently, Bolt has five different modes of running code:

1. build

```bash
bolt build in -o out
```
Writes the bytecode

2. debug

```bash
bolt debug in -o out
```
Writes the bytecode and prints the tokens and the AST

3. time

```bash
bolt time in -o out
```
Writes the bytecode and prints the time taken at every stage

4. direct

```bash
bolt -d "var name = 'a';" -o out
```

5. check

```bash
bolt check in -o out
```
Useful for IDE/editor checkers, builds and checks the AST, but generates no bytecode.

---

In all three modes, `in` is `main.blt` if not specified and `out`, `main.bvm`.

These modes are not currently combinable. None of this modes executes it yet.

## Expected Syntax

> Syntax may vary, as Bolt is in an early stage of development

```
fn add(a: i32, b: i32) -> i32 {
    ret a+b;
}

var n1: int = 123; // int is an alias for i32
var n2: int = 494;

print(add(n1, n2)); // This print method is a placeholder for future print methods

```

### write

```
write "string or char";
```

### variables

```
var name = 'a';
var points: i16 = 45;
```

Write and simple variables are working!

## How to compile and run your program

1. Write your program

We will use this as an example:

`main.blt`
```
write "hello, world!";
```

2. Compile with Bolt

```bash
bolt run main.blt -o main.bvm
```

3. Move the resulting files (`main.bvm` and `BVM-CPOOL`) to the `vm/` folder

4. Run the bytecode

```bash
cd vm
bvm
```

You will see something like this:
```
hello, world!

VM Execution time: 130000 nanoseconds.
Or: 0.1300 ms.
```

> Note: the vm uses `puts` and reads the CPOOL file, so using write is kinda slow