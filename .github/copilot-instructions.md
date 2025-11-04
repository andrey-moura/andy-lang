# Copilot Instructions for Andy Language

## Project Overview

Andy is an interpreted object-oriented multi-purpose programming language written in C++. The project includes:

- An interpreter for the Andy language
- A standard library with built-in classes and functions
- UI support via built-in extensions
- Cross-platform support (Linux, Windows, WebAssembly)
- VSCode extension for language support

## Project Structure

```
andy-lang/
├── src/              # Core interpreter implementation
│   ├── api.cpp       # Public API for the interpreter
│   ├── interpreter.cpp
│   ├── lexer.cpp     # Lexical analysis
│   ├── parser.cpp    # Syntax parsing
│   ├── classes/      # Built-in class implementations
│   └── builtin_libs/ # Built-in extensions (drawing, UI)
├── include/          # Public header files
│   └── andy/lang/    # Core language headers
├── modules/          # Modular components
│   ├── andy/         # Main interpreter module
│   ├── andy-core/    # Core utilities
│   ├── andy-console/ # Console I/O
│   └── andy-widgets/ # UI widgets
├── tests/            # Test suite
│   ├── andy-lang/    # C++ unit tests
│   └── high-level/   # Andy language test specs
└── examples/         # Example Andy programs
```

## Build System

The project uses **CMake** (minimum version 3.10) with C++20 standard.

### Building the Project

```bash
cmake -DCMAKE_BUILD_TYPE=Release -B build .
cmake --build build --config Release --parallel
```

### Installing

```bash
sudo cmake --install build --config Release  # Linux
# Or run with Administrator privileges on Windows
```

### Running Tests

```bash
build/andy-test tests
```

Tests are written in C++ (using a custom testing framework) and in Andy language itself.

## Coding Standards

### C++ Code

- **Standard**: C++20
- **Style**: 
  - Use 4 spaces for indentation (no tabs)
  - Opening braces on the following line for functions and classes
  - Use `snake_case` for variables, functions and class names
  - Use namespace `andy::lang` for language components
  - Prefer `std::shared_ptr` for object management
  
### Header Files

- Use `#pragma once` instead of include guards
- Headers in `include/andy/lang/` are public API
- Use forward declarations when possible
- Include documentation comments using `/// @brief` style

### Example Code Pattern

```cpp
namespace andy
{
    namespace lang
    {
        /// @brief Executes the code in a file.
        /// @param path The path to the source code.
        /// @return Returns a shared pointer to the object.
        std::shared_ptr<andy::lang::object> evaluate(std::filesystem::path path);
    }
}
```

## Key Components

### Lexer (`src/lexer.cpp`)
Tokenizes Andy source code. Handles:
- String/number literals
- Operators and delimiters
- Comments (single-line with `//`)
- Identifiers and keywords

### Parser (`src/parser.cpp`)
Builds an Abstract Syntax Tree (AST) from tokens.
Supports function calls with or without parentheses.

### Interpreter (`src/interpreter.cpp`)
Executes the AST. Manages:
- Object instances
- Class definitions
- Method calls
- Variable scoping

### Object System (`src/object.cpp`)
Implements the object-oriented features:
- All values are objects
- Built-in classes: String, Number, Array, Dictionary, File, etc.
- Instance and class variables
- Method dispatch

## Andy Language Features

- Object-oriented with classes
- Dynamic typing
- Functions can be called with or without parentheses
- Keywords: `class`, `end`, `fn`, `if`, `loop`, `var`, `new`
- Built-in output: `out 'text'`
- See `SPECIFICATION.md` for detailed language specification

## Important Notes

### When Working on Parser/Lexer
- The language supports function calls without parentheses (e.g., `out 'Hello'`)
- Be careful with operator precedence
- Comments are single-line only (`//`)

### When Working on Built-in Classes
- Classes are in `src/classes/`
- Each class typically has instance methods and class methods
- Use the interpreter's class registry for lookups

### When Adding Features
- Update `SPECIFICATION.md` if adding language features
- Add tests in `tests/high-level/` for language features
- Add C++ tests in `tests/andy-lang/` for interpreter internals
- Ensure cross-platform compatibility (Linux, Windows, WASM)

## Continuous Integration

The project uses GitHub Actions with workflows for:
- Ubuntu 18.04, 22.04, 24.04
- Windows Server 2022
- WebAssembly builds

All CI workflows:
1. Install dependencies
2. Configure CMake
3. Build the project
4. Install and verify version
5. Run tests (continue-on-error: true)
6. Upload artifacts

## Common Tasks

### Adding a New Built-in Class
1. Create `src/classes/your_class.cpp`
2. Implement the class methods
3. Register in the interpreter
4. Add tests in `tests/high-level/`

### Modifying the Lexer
1. Update token recognition in `src/lexer.cpp`
2. Update `SPECIFICATION.md` if needed
3. Add lexer tests in `tests/andy-lang/lexer_spec.cpp`

### Adding a Module
1. Create directory in `modules/`
2. Add `CMakeLists.txt`
3. Include in root `CMakeLists.txt`

## Dependencies

The project uses Git submodules for dependencies. Always use `--recursive` when cloning:

```bash
git clone https://github.com/andrey-moura/andy-lang --recursive
```

## Version Management

Version is stored in the `VERSION` file at the root and follows semantic versioning with optional `-alpha` or `-beta` suffixes.
