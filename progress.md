# Progress

This document records each development phase: execution target, execution
summary, and execution result.

## Phase 0: Repository Bootstrap

### Execution Target

Create the initial GitHub project foundation and document the long-term
compiler direction.

### Execution Summary

- Created `README.md`.
- Created `arch.md`.
- Created `plan.md`.
- Defined the final project goal: an AI self made compiler based on RISCV RVV
  accelerator.
- Pushed the initial documentation to GitHub.

### Execution Result

Completed.

Commit:

```text
fa08048 Add initial compiler architecture docs
```

## Phase 1: Build System and Tool Skeleton

### Execution Target

Create a minimal CMake-based compiler driver that links against the existing
local LLVM/MLIR build.

### Execution Summary

- Added top-level `CMakeLists.txt`.
- Added `tools/zc/zc.cpp`.
- Added `tools/zc/CMakeLists.txt`.
- Added `.gitignore`.
- Added `examples/hello.zc`.
- Connected the project to:
  - `/home/zyz/mlir/build/lib/cmake/llvm`
  - `/home/zyz/mlir/build/lib/cmake/mlir`
- Implemented initial CLI options:
  - `--emit=tokens`
  - `--emit=ast`
  - `--emit=mlir`
  - `--emit-tokens`
  - `--emit-ast`
  - `--emit-mlir`
  - `-o <file>`

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
/home/zyz/zcomipler/build/tools/zc/zc --help
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
```

Commit:

```text
865643f Add phase1 compiler driver skeleton
```

## Phase 2: Lexer

### Execution Target

Convert `.zc` source text into a token stream.

### Execution Summary

- Added `TokenKind` and `Token`.
- Added `Lexer`.
- Added keyword recognition for `func`, `let`, `return`, and `i32`.
- Added identifier and integer literal recognition.
- Added punctuation and operator recognition.
- Added `//` comment skipping.
- Added invalid character diagnostics with line and column.
- Implemented `zc --emit-tokens`.
- Added lexer CTest coverage.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-tokens
```

Commit:

```text
c0e275d Add phase2 lexer
```

## Phase 3: Parser and AST

### Execution Target

Parse tokens into a structured AST and expose the result through
`zc --emit-ast`.

### Execution Summary

- Added AST classes for:
  - module
  - function
  - let statement
  - return statement
  - integer expression
  - variable expression
  - binary expression
- Added a recursive descent parser.
- Added operator precedence parsing for `+`, `-`, `*`, and `/`.
- Implemented `zc --emit-ast`.
- Added parser CTest coverage for valid and invalid input.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-ast
```
