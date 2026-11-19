# Contributing to Bolt

First off, thank you for considering contributing to Bolt! It's open-source projects like this that make the developer community such a great place to learn, inspire, and create.

Bolt is an experimental programming language written in C++ that transpiles to BVM bytecode. Whether you are fixing a bug, improving documentation, adding new syntax features, or writing tests, your help is warmly welcomed!

---

## Code of Conduct & AI Policy

- **Respect & Kindness:** Please ensure all interactions in issues and pull requests remain respectful and collaborative.
- **AI Policy:** Contributions generated using AI tools (LLMs, Copilot, etc.) are permitted **only if you are honest about it and the PR is good** else, your PR will be rejected.

---

## Setting Up the Development Environment

To build and contribute to Bolt, you need some tools installed:

### Prerequisites

- **C/C++ Compiler:** `clang` or `gcc` (C17 and C++17)
- **Cmake** (or the **deprecated** Python builder)
- **Git:** For version control

### Building Bolt Locally

1. **Clone the repository:**
   ```bash
   git clone https://github.com/Pacsfury/bolt-lang.git
   cd bolt-lang
   ```

2. **Compile Bolt directly:**
   ```bash
   cmake -S . -B cmake-build
   cmake --build cmake-build --config Release
   ```

4. **Run a test Bolt file:**
   ```bash
   ./bolt build examples/io.blt
   ./bvm
   ```

---

## How to Contribute

### 1. Pick an Issue or Feature
Check the **[GitHub Issues](https://github.com/Pacsfury/bolt-lang/issues)** tab for tasks labeled:
- `good first issue`: Ideal for newcomers to the codebase.
- `help wanted`: Features or bug fixes requiring community assistance.
- `documentation`: Help improve the README, `DESIGN.md`, or guides.

> *If you want to work on a new feature not listed in issues, please open an issue first to discuss the design before writing code!*

### 2. Understand the Basic Architecture

```
Source Code (.blt) ➔ Lexer (frontend/lexer/tokens.cpp) ➔ ASTgen (frontend/astgen/astgen.cpp) ➔ Checker (frontend/checker/structure.cpp) ➔ Codegen(backend/codegen/codegen.cpp) ➔ BVM (vm/executor.c)
```


### 3. Workflow & Pull Requests

1. **Fork the repository** on GitHub.
2. **Create a feature branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```
3. **Make your changes** while adhering to the C/C++ coding conventions listed below.
4. **Test your code:** Ensure existing examples still compile and run properly.
5. **Commit your changes:** Write clear, concise commit messages.
   ```bash
   git commit -m "feat(ast): add support for function argument parsing"
   ```
6. **Push to your fork & submit a PR:**
   - Describe what changed and why.
   - Link any related issue (e.g., `Fixes #12`).

## Coding Standards

- **Language:** Clean standard C++17 and C17. Avoid compiler-specific non-standard extensions.
- **Formatting:** Use 4 spaces for indentation. Keep functions focused and modular.

---

## Need Help?

If you have questions or get stuck while contributing, feel free to open a Q&A issue or ask directly on active Pull Request threads.
