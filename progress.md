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

## Phase 17B: Scalar Memory and Array Access

### Execution Target

Add the scalar memory baseline needed before vector syntax: typed buffer
parameters, scalar indexed loads, and scalar indexed stores.

### Execution Summary

- Extended the Phase 17 design with the `ptr<i32>` source type.
- Added lexer support for:
  - `ptr`
  - `load`
  - `store`
  - `[`
  - `]`
- Added AST nodes:
  - `LoadExprAST`
  - `StoreStmtAST`
- Added parser support for:
  - `ptr<i32>` parameter types
  - `load a[i]`
  - `store c[i] = value;`
- Added MLIRGen lowering:
  - `ptr<i32>` -> `memref<?xi32>`
  - `load` -> `memref.load`
  - `store` -> `memref.store`
  - `i32` index -> `index` through `arith.index_cast`
- Extended reference LLVM/RISC-V emitters for typed-pointer load/store.
- Hardened the RISC-V backend fallback: when MLIR v23 LLVM IR is incompatible
  with the system LLVM 14 `llc`, the backend retries with typed-pointer
  reference LLVM IR and still emits RISC-V assembly through `llc`.
- Added `examples/arrays.zc`.
- Added lexer, parser, MLIR, LLVM IR, and RISC-V assembly golden tests for
  `arrays.zc`.

### Execution Result

Completed.

This phase provides the scalar memory shape needed for future vector add:

```zc
func add_at(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, i: i32) -> i32 {
  let x = load a[i];
  let y = load b[i];
  let z = x + y;
  store c[i] = z;
  return z;
}
```

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-riscv-asm
/home/zyz/mlir/build/bin/mlir-opt /tmp/arrays.mlir -o /tmp/arrays.checked.mlir
/home/zyz/mlir/build/bin/llvm-as /tmp/arrays.ll -o /tmp/arrays.bc
riscv64-linux-gnu-as /tmp/arrays.s -o /tmp/arrays.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

Commit:

```text
fdf4299 Implement phases 4 through 7 codegen
d150b2e Track codegen golden outputs
```

## Phase 8: RISC-V Assembly

### Execution Target

Generate RISC-V assembly for the first toy program.

### Execution Summary

- Added `zc --emit-riscv-asm`.
- Added RISC-V text emission for integer constants, arithmetic, comparison,
  `if`, `while`, and `return`.
- Added `examples/control.zc` and `examples/while.zc`.
- Added golden tests for RISC-V assembly.
- Added optional validation with `riscv64-linux-gnu-as` when available.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-riscv-asm
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 9: Control Flow

### Execution Target

Add basic control flow and comparison operators.

### Execution Summary

- Added keywords:
  - `if`
  - `else`
  - `while`
- Added comparison operators:
  - `<`
  - `<=`
  - `>`
  - `>=`
  - `==`
  - `!=`
- Added AST nodes:
  - `IfStmtAST`
  - `WhileStmtAST`
- Added parser support for if/else blocks and while blocks.
- Added LLVM IR control-flow emission with labels and branches.
- Added parser and codegen tests for control-flow examples.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/control.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/control.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/while.zc --emit-llvm
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 10: Vector and RVV Preparation

### Execution Target

Define a concrete path from future source-level vector operations to RISC-V RVV
code generation.

### Execution Summary

- Added [docs/rvv.md](docs/rvv.md).
- Proposed future vector source syntax.
- Defined the planned lowering path:
  - zc vector source syntax
  - zc vector operations
  - MLIR vector dialect
  - LLVM dialect / RVV intrinsics
  - RISC-V RVV assembly
- Listed initial vector operation candidates.
- Listed first RVV instruction families to target.

### Execution Result

Completed as a design and implementation-preparation phase.

## Phase 11: AI-Assisted Compiler Direction

### Execution Target

Define the first AI-assisted compiler workflow for optimization experiments,
benchmark records, prompt records, and safety rules.

### Execution Summary

- Added [docs/ai-workflow.md](docs/ai-workflow.md).
- Defined experiment record format.
- Defined prompt record format.
- Added safety rules for AI-generated compiler changes.
- Listed near-term AI-assisted compiler uses.

### Execution Result

Completed as a workflow foundation.

Final validation:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 12: Registered zc MLIR Dialect

### Execution Target

Turn the initial textual `zc` dialect surface into a registered MLIR dialect.

### Execution Summary

- Added TableGen-backed `ZCDialect`.
- Added registered `zc` operations:
  - `zc.constant`
  - `zc.add`
  - `zc.sub`
  - `zc.mul`
  - `zc.div`
- Added generated dialect/op headers and implementation files.
- Added `zc-opt`, a small MLIR optimizer driver that registers the `zc`,
  `arith`, and `func` dialects.
- Added dialect parser/printer test input.

### Execution Result

Completed for core arithmetic operations.

Note: this phase intentionally keeps functions in standard `func.func` while
`zc` owns computation operations. This keeps the first registered dialect small
and makes Phase 13 lowering clearer.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc-opt/zc-opt /home/zyz/zcomipler/test/dialect/registered.mlir -o /tmp/registered.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 13: Real MLIR Lowering Pass

### Execution Target

Lower registered `zc` arithmetic operations to standard MLIR `arith`
operations using an MLIR pass.

### Execution Summary

- Added `ZCToStandard` conversion library.
- Added `--lower-zc-to-standard` pass registration.
- Lowered:
  - `zc.constant` -> `arith.constant`
  - `zc.add` -> `arith.addi`
  - `zc.sub` -> `arith.subi`
  - `zc.mul` -> `arith.muli`
  - `zc.div` -> `arith.divsi`
- Extended dialect tests to assert lowered output contains no `zc.`
  operations.

### Execution Result

Completed for core arithmetic operations.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc-opt/zc-opt /home/zyz/zcomipler/test/dialect/registered.mlir --lower-zc-to-standard -o /tmp/lowered.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 14: AST to In-Memory MLIR

### Execution Target

Move the primary `--emit-mlir` path from hand-written MLIR text to MLIR C++ API
construction.

### Execution Summary

- Added `MLIRGen` module.
- Kept AST independent from MLIR headers.
- Built `mlir::ModuleOp` with `MLIRContext` and `OpBuilder`.
- Emitted `func.func`, `arith.constant`, arithmetic operations, comparisons,
  and `func.return` through MLIR operation builders.
- Updated `zc --emit-mlir` to print `ModuleOp`.
- Updated MLIR golden tests for MLIR printer-owned SSA names.

### Execution Result

Completed for straight-line arithmetic programs.

Note: `if` and `while` in-memory MLIR generation are intentionally deferred to
the vector/control-flow hardening path. Textual LLVM/RISC-V paths still support
the current control-flow examples.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/hello.mlir -o /tmp/hello.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 15: MLIR to LLVM IR Pipeline

### Execution Target

Move the straight-line `--emit-llvm` path onto the official MLIR lowering and
translation tools instead of relying only on hand-written LLVM IR text.

### Execution Summary

- Added an MLIR-backed LLVM IR emission path in the `zc` driver.
- The driver now builds an in-memory MLIR module, writes it to a temporary MLIR
  file, invokes:
  - `/home/zyz/mlir/build/bin/mlir-opt --convert-to-llvm`
  - `/home/zyz/mlir/build/bin/mlir-translate --mlir-to-llvmir`
- Kept the existing text LLVM IR emitter as a fallback for language features
  that are not yet covered by in-memory MLIR generation, such as current
  `if`/`while` examples.
- Updated the `hello.zc` LLVM IR golden output to match MLIR's LLVM dialect
  translation result.

### Execution Result

Completed for straight-line arithmetic programs.

Note: full control-flow lowering through in-memory MLIR is intentionally kept as
a later hardening phase. The existing control-flow LLVM tests continue to pass
through the fallback path.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-llvm
/home/zyz/mlir/build/bin/llvm-as /tmp/new-hello.ll -o /tmp/new-hello.bc
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 16: LLVM RISC-V Backend Integration

### Execution Target

Generate RISC-V assembly through LLVM's RISC-V backend while keeping the
hand-written RISC-V emitter available as a reference fallback.

### Execution Summary

- Added a new `Target/RiscV` backend module.
- Moved the MLIR-backed LLVM IR emission helper behind a reusable target-layer
  API.
- Implemented the backend path:
  - AST
  - in-memory MLIR
  - `mlir-opt --convert-to-llvm`
  - `mlir-translate --mlir-to-llvmir`
  - `llc -mtriple=riscv64-unknown-elf -mattr=+m`
  - RISC-V assembly
- Updated `zc --emit-riscv-asm` to prefer the LLVM backend path and fall back
  to the hand-written reference emitter if the backend path is unavailable.
- Updated architecture documents and the architecture diagram to show the
  `Target/RiscV` module.
- Updated the `hello.zc` RISC-V assembly golden output to match LLVM backend
  output.

### Execution Result

Completed for straight-line arithmetic programs.

Environment note: `/home/zyz/mlir/build/bin/llc` currently does not include a
registered RISC-V target, so this phase uses `/usr/bin/llc`, which does include
`riscv64`. The MLIR conversion and LLVM IR translation still use the local MLIR
build under `/home/zyz/mlir/build/bin`.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-riscv-asm
riscv64-linux-gnu-as /tmp/new-hello.riscv -o /tmp/new-hello.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 17: Functions, Calls, and Assignment

### Execution Target

Extend the language toward simple kernel structure with function parameters,
function calls, and straight-line assignment while documenting the next memory
model step.

### Execution Summary

- Added [docs/phase17-functions-memory.md](docs/phase17-functions-memory.md).
- Added lexer tokens for `:` and `,`.
- Added AST support for:
  - function parameters
  - call expressions
  - assignment statements
- Extended the parser to support:
  - `func add(a: i32, b: i32) -> i32`
  - `add(2, 3)` expression calls
  - `x = x + 4;` straight-line assignment
- Extended text emitters and MLIRGen for parameters, calls, and straight-line
  assignment.
- Added `examples/calls.zc`.
- Added lexer, parser, MLIR, LLVM IR, and RISC-V assembly golden coverage for
  `calls.zc`.

### Execution Result

Completed for the first Phase 17 slice.

Note: this phase intentionally models assignment as a new value bound to the
same source name in straight-line code. Explicit array/memref load-store syntax
is documented as the next memory-model slice.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-riscv-asm
/home/zyz/mlir/build/bin/mlir-opt /tmp/calls.mlir -o /tmp/calls.checked.mlir
/home/zyz/mlir/build/bin/llvm-as /tmp/calls.ll -o /tmp/calls.bc
riscv64-linux-gnu-as /tmp/calls.s -o /tmp/calls.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```
