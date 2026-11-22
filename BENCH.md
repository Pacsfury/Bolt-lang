# Benchmarks

> **Environment:** Intel i5 / DDR5 / Win11 / MSVC  
> **Build Command:** `cmake --build cmake-build --config Release`  
> **Date:** 2026-11-15


Every benchmark includes:
- Executed source code
- Compilation breakdown (`bolt time <file>`)
- Bolt Virtual Machine (BVM) execution time

*Note: BVM execution time is omitted for benchmarks containing `read` to exclude interactive I/O wait times.*


## Summary

| Benchmark | Lines | Total Compile Time | Throughput | BVM Execution |
| :--- | :---: | :---: | :---: | :---: |
| **1. Output** | 6 | 0.0374 ms | ~160k LOC/s | 0.0790 ms |
| **2. IO & Mutation** | 12 | 0.0326 ms | ~368k LOC/s | *N/A (`read`)* |
| **3. Arithmetic** | 7 | 0.0656 ms | ~107k LOC/s | 0.0765 ms |
| **4. Mixed String Pool** | 20 | 0.0411 ms | ~487k LOC/s | *N/A (`read`)* |
| **5. 5,000-Line Stress** | 4,999 | 3.0280 ms | **~1.65M LOC/s** | 0.0685 ms |
| **6. 1,000 If-Else Stress** | 2,001 | 2.0908 ms | ~957k LOC/s | 0.0203 ms |
| **7. Fixed-Domain Sorter** | 707 | 0.3148 ms | **~2.25M LOC/s** | 0.0499 ms |


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

## Benchmark 5: Really long thingy

**Generator**
```bash
{
  echo 'const base = 42;'
  echo 'var acc = 10;'
  for ((i=1; i<=2498; i++)); do
    echo "var v$i = $i * 3 + 12 / 4 - 2;"
    echo "acc = acc + v$i * 2 - 1;"
  done
  echo 'acc;'
} > stress.blt
```

```powershell
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('const base = 42;')
$lines.Add('var acc = 10;')
for ($i = 1; $i -le 2498; $i++) {
    $lines.Add("var v$i = $i * 3 + 12 / 4 - 2;")
    $lines.Add("acc = acc + v$i * 2 - 1;")
}
$lines.Add('acc;')
[System.IO.File]::WriteAllLines("$PWD\stress.blt", $lines)
```

**Compilation**
```
Lexing time: 1149600 nanoseconds.
Or: 1.1496 ms.
ASTgen time: 1305100 nanoseconds.
Or: 1.3051 ms.
Checking time: 196500 nanoseconds.
Or: 0.1965 ms.
Codegen time: 376800 nanoseconds.
Or: 0.3768 ms.
```

**Execution**
```
VM Execution time: 68500 nanoseconds.
Or: 0.0685 ms.
```

## Benchmark 6: 1,000 If-Else Stress

Generates 1,000 `if`/`else` statements, alternating between the two branches. No input is read.

**Bash generator**
```bash
{
  for ((i=1; i<=1000; i++)); do
    if ((i % 2 == 0)); then value="yes"; else value="no"; fi
    echo "var flag_$i = \"$value\";"
    echo "if flag_$i == \"yes\" { flag_$i = \"taken\"; } else { flag_$i = \"free\"; }"
  done
  echo 'write "If-else benchmark complete.";'
} > if-heavy.blt
```

**PowerShell generator**
```powershell
$lines = [System.Collections.Generic.List[string]]::new()
for ($i = 1; $i -le 1000; $i++) {
    $value = if ($i % 2 -eq 0) { 'yes' } else { 'no' }
    $lines.Add("var flag_$i = `"$value`";")
    $lines.Add("if flag_$i == `"yes`" { flag_$i = `"taken`"; } else { flag_$i = `"free`"; }")
}
$lines.Add('write "If-else benchmark complete.";')
[System.IO.File]::WriteAllLines("$PWD\if-heavy.blt", $lines)
```

**Compilation**
```
Lexing time: 241100 nanoseconds.
Or: 0.2411 ms.
ASTgen time: 1048100 nanoseconds.
Or: 1.0481 ms.
Checking time: 531700 nanoseconds.
Or: 0.5317 ms.
Codegen time: 269900 nanoseconds.
Or: 0.2699 ms.
```

**Execution**
```
VM Execution time: 20300 nanoseconds.
Or: 0.0203 ms.
```

## Benchmark 7: Fixed-Domain Sorter

Bolt currently has no arrays, loops, or relational comparisons. This benchmark uses generated, unrolled bubble-sort passes over the fixed string values `"1"` through `"5"`, checking each possible inverted pair with `if`. The initial order is `5, 1, 4, 2, 3`.

**Bash generator**
```bash
{
  printf 'var item_1 = "5";\nvar item_2 = "1";\nvar item_3 = "4";\nvar item_4 = "2";\nvar item_5 = "3";\nvar temp = "";\n'
  for ((pass=0; pass<4; pass++)); do
    for ((position=1; position<=4-pass; position++)); do
      for ((left=2; left<=5; left++)); do
        for ((right=1; right<left; right++)); do
          printf 'if item_%d == "%d" {\n' "$position" "$left"
          printf '    if item_%d == "%d" {\n' "$((position + 1))" "$right"
          printf '        temp = item_%d;\n' "$position"
          printf '        item_%d = item_%d;\n' "$position" "$((position + 1))"
          printf '        item_%d = temp;\n' "$((position + 1))"
          printf '    }\n}\n'
        done
      done
    done
  done
  echo 'write "Sort benchmark complete.";'
} > sorter.blt
```

**PowerShell generator**
```powershell
$lines = [System.Collections.Generic.List[string]]::new()
$initialValues = @(5, 1, 4, 2, 3)
for ($i = 0; $i -lt $initialValues.Count; $i++) {
    $lines.Add(('var item_{0} = "{1}";' -f ($i + 1), $initialValues[$i]))
}
$lines.Add('var temp = "";')

for ($pass = 0; $pass -lt 4; $pass++) {
    for ($position = 1; $position -le 4 - $pass; $position++) {
        for ($left = 2; $left -le 5; $left++) {
            for ($right = 1; $right -lt $left; $right++) {
                $lines.Add(('if item_{0} == "{1}" {{' -f $position, $left))
                $lines.Add(('    if item_{0} == "{1}" {{' -f ($position + 1), $right))
                $lines.Add(('        temp = item_{0};' -f $position))
                $lines.Add(('        item_{0} = item_{1};' -f $position, ($position + 1)))
                $lines.Add(('        item_{0} = temp;' -f ($position + 1)))
                $lines.Add('    }')
                $lines.Add('}')
            }
        }
    }
}
$lines.Add('write "Sort benchmark complete.";')
[System.IO.File]::WriteAllLines("$PWD\sorter.blt", $lines)
```

**Compilation**
```
Lexing time: 41900 nanoseconds.
Or: 0.0419 ms.
ASTgen time: 185500 nanoseconds.
Or: 0.1855 ms.
Checking time: 46400 nanoseconds.
Or: 0.0464 ms.
Codegen time: 41000 nanoseconds.
Or: 0.041 ms.
```

**Execution**
```
VM Execution time: 49900 nanoseconds.
Or: 0.0499 ms.
```

> Important: none of this benchmarks uses constant folding, DCE, etc, as these will come in a future module (optimizer) executed only with the -O1 or -O2 flags!

## Contribute a Benchmark

Want to share how Bolt runs on your machine?

1. Create a `benchmark/` folder in the root directory (if it doesn't exist yet).
2. Run the exact same benchmark files in **Release** mode and copy this template with your own environment details, date, and timings.
3. Name your file `BENCH-<OS>-<CPU>-<COMPILER>.md` (for example, `BENCH-WIN11-I5-MSVC.md`).
4. Open a Pull Request!

> Important! Follow naming conventions already used by other files and don't repeat a benchmark already done! If you do, update it if it's too old, but don't create a new file for it!