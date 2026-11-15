# Benchmarks

Every benchmark includes:
- Executed code
- Time (bolt time <file>)
- BVM time

All benchmarks are executed with executables generated with:
```bash
cmake --build cmake-build --config Release
```

> Note: If the benchmark includes 'read' we won't write the VM time taken

## Benchmark 1: Output

**Input**
```
var value = "Welcome to Bolt";
var valu3 = "This is a benchmark";
write "Starting benchmark #1...";
write value;
write valu3;
write "Benchmark #1 ended.";
```
**Compilation**
```
Lexing time: 2700 nanoseconds.
Or: 0.0027 ms.
ASTgen time: 14300 nanoseconds.
Or: 0.0143 ms.
Checking time: 800 nanoseconds.
Or: 0.0008 ms.
Codegen time: 19600 nanoseconds.
Or: 0.0196 ms.
```
**Execution**
```
VM Execution time: 79000 nanoseconds.
Or: 0.0790 ms.
```

## Benchmark 2: IO and variable mutation

**Input**
```
var greeting = "Hello, ";
var user_input = "";
var status = "Initialized";

write "Enter a token:";
read user_input;

status = "Processing";
write greeting;
write user_input;
write status;

status = "Completed";
write status;
```

**Compilation**
```
Lexing time: 2900 nanoseconds.
Or: 0.0029 ms.
ASTgen time: 14500 nanoseconds.
Or: 0.0145 ms.
Checking time: 2000 nanoseconds.
Or: 0.002 ms.
Codegen time: 13200 nanoseconds.
Or: 0.0132 ms.
```

## Benchmark 3: Arithmetic
> Note that write doesn't support ints

**Input**
```
const base = 100;
var step1 = 1 + 4 * 6 / 1 + 3 * 6 * 12 - 54 + 342 * 34;
var step2 = 999 * 12 / 3 + 450 - 123 * 4 + 88 / 2 * 19;
var step3 = 12 * 12 * 12 - 500 / 5 + 33 * 77 - 91 + 14 * 6;
var total = 10 + 20 * 30 - 40 / 2 + 5 * 5 * 5 - 15 + 8 * 9 * 10 - 100 + 250 * 4;

write "Arithmetic benchmark complete.";
total;
```

**Compilation**
```
Lexing time: 4600 nanoseconds.
Or: 0.0046 ms.
ASTgen time: 38900 nanoseconds.
Or: 0.0389 ms.
Checking time: 15000 nanoseconds.
Or: 0.015 ms.
Codegen time: 7100 nanoseconds.
Or: 0.0071 ms.
```

**Execution**
```
VM Execution time: 76500 nanoseconds.
Or: 0.0765 ms.
```

## Benchmark 4: Mixed String Pool, Constants & High Statement Count

**Input**
```
const header = "=== BOLT STRESS TEST ===";
const footer = "=== END OF TEST ===";

var s1 = "Alpha";
var s2 = "Bravo";
var s3 = "Charlie";
var s4 = "Delta";
var input_val = "";

write header;
write s1;
write s2;
read input_val;

s1 = "Echo";
s2 = "Foxtrot";
s3 = "Golf";
s4 = "Hotel";

write input_val;
write s1;
write s2;
write s3;
write s4;
write footer;
```

**Compilation**
```
Lexing time: 3500 nanoseconds.
Or: 0.0035 ms.
ASTgen time: 22600 nanoseconds.
Or: 0.0226 ms.
Checking time: 2000 nanoseconds.
Or: 0.002 ms.
Codegen time: 13000 nanoseconds.
Or: 0.013 ms.
```
