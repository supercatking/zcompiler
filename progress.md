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

Commit:

```text
9a9c88a Add phase3 parser and AST
```

## Phase 4: First MLIR Emission

### Execution Target

Generate the first standard MLIR output from the AST.

### Execution Summary

- Added `CodeGen` module.
- Added AST read-only accessors for code generation.
- Implemented standard MLIR text emission through `zc --emit-mlir`.
- Emitted builtin `module`, `func.func`, `arith.constant`, arithmetic ops, and
  `return`.
- Added codegen golden tests.
- Validated generated standard MLIR with `/home/zyz/mlir/build/bin/mlir-opt`.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/hello.mlir -o /tmp/hello.opt.mlir
```

## Phase 5: Custom zc Dialect

### Execution Target

Introduce the first project-owned `zc` dialect surface.

### Execution Summary

- Added initial ODS/TableGen design files:
  - `include/zcompiler/Dialect/ZC/IR/ZCDialect.td`
  - `include/zcompiler/Dialect/ZC/IR/ZCOps.td`
- Implemented `zc --emit-zc-mlir`.
- Added textual `zc` operations:
  - `zc.func`
  - `zc.constant`
  - `zc.add`
  - `zc.sub`
  - `zc.mul`
  - `zc.div`
  - `zc.return`
- Added golden tests for emitted `zc` MLIR text.

### Execution Result

Completed as the first vertical-slice dialect surface.

Note: this phase currently emits the project-owned dialect text and includes an
ODS scaffold. A later hardening pass should compile/register the dialect with
MLIR TableGen so `mlir-opt` can parse `zc` operations directly.

Validated command:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-zc-mlir
```

## Phase 6: Lowering Passes

### Execution Target

Lower the `zc` dialect surface to standard MLIR operations.

### Execution Summary

- Implemented `zc --emit-lowered-mlir`.
- Mapped `zc` arithmetic operations to `arith` operations.
- Mapped `zc.func` and `zc.return` to standard `func` dialect syntax.
- Added tests that verify lowered MLIR no longer contains `zc.` operations.
- Validated lowered MLIR with the same standard MLIR output path.

### Execution Result

Completed as AST-backed textual lowering.

Note: the next hardening step is to move this lowering into MLIR rewrite
patterns after the compiled `zc` dialect is registered.

Validated command:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-lowered-mlir
```

## Phase 7: LLVM IR Emission

### Execution Target

Emit LLVM IR for the first toy program.

### Execution Summary

- Implemented `zc --emit-llvm`.
- Generated a valid `define i32 @main()` function.
- Lowered integer arithmetic to LLVM IR instructions:
  - `add`
  - `sub`
  - `mul`
  - `sdiv`
- Added LLVM IR golden tests.
- Validated generated LLVM IR with `/home/zyz/mlir/build/bin/llvm-as`.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-llvm
/home/zyz/mlir/build/bin/llvm-as /tmp/hello.ll -o /tmp/hello.bc
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```
