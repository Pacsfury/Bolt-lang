# CI checks

The `CMake checks` workflow runs on pushes, pull requests, and manual dispatch.
Both jobs run on Ubuntu, macOS, and Windows.

## Language checks

- Builds the `bolt` compiler with CMake.
- Runs `bolt check` on the existing `examples/math.blt` and `examples/io.blt`
  programs to check that the language frontend accepts them.

## VM smoke test

- Builds both `bolt` and `bvm` with CMake.
- Compiles the existing `examples/math.blt` program and runs it with BVM.
- Checks that BVM completes execution and prints its execution-time output.
  The arithmetic example returns a non-zero result, so the workflow expects a
  non-zero process exit code.

These are smoke checks using existing examples; the workflow does not add a
separate test suite or new language programs.
