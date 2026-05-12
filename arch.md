# Toy Compiler Architecture

This document defines the first architecture of `zcompiler`, a small
MLIR-based toy compiler that will grow toward an AI-assisted RISC-V RVV compiler.

## Architecture Diagram

![zcompiler architecture](arch.svg)

The diagram shows the current project shape after the toy compiler phases:

- The frontend owns `.zc` source parsing: lexer, parser, and AST.
- The AST is the central in-memory program model used by later compiler stages.
- The `CodeGen` module keeps the early textual reference emitters.
- The `MLIRGen` and `Target/RiscV` modules own the infrastructure-backed path:
  AST -> in-memory MLIR -> LLVM IR -> LLVM RISC-V backend assembly.
- The RVV and AI-assisted blocks are deliberate future-facing architecture
  areas. They are documented now so the toy compiler can grow toward the final
  accelerator-oriented goal without changing direction later.
- Accelerator profiles under `profiles/` capture target assumptions such as
  `rv64gcv`, SEW/LMUL policy, tail/mask policy, and current backend strategy.
- Phase 31T adds `matrix_multiply` as a dedicated AST/kernel statement so matrix
  semantics have a clean hook for future RVV tiling and packed-memory lowering.
- Phase 31U adds `matrix_multiply_packed_b`, making RHS layout explicit and
  enabling direct RVV dot-product lowering over unit-stride packed columns.

## 1. Design Principles

The first compiler must be small, testable, and easy to change. The goal is not
to build a complete production compiler immediately, but to create a clean
vertical path from a tiny source language to RISC-V assembly.

Core principles:

- Use MLIR as the central intermediate representation.
- Keep the frontend language intentionally small.
- Prefer clear lowering stages over one large translation step.
- Use LLVM's existing RISC-V backend instead of writing a backend from scratch.
- Keep every stage visible and testable from the command line.
- Design the first custom dialect so it can later grow toward tensor, vector,
  and RVV-specific optimization.
- Document architecture before implementation when a phase changes module
  boundaries.
- Keep parser, AST, MLIR dialect, lowering, backend, benchmark, and AI workflow
  layers decoupled.
- Add tests with every phase and record difficult unresolved bugs in
  `known_issue.md`.

## 2. High-Level Pipeline

```text
          +----------------+
          | source .zc file |
          +--------+-------+
                   |
                   v
          +----------------+
          | lexer / parser |
          +--------+-------+
                   |
                   v
          +----------------+
          |      AST       |
          +--------+-------+
                   |
                   v
          +----------------+
          | zc MLIR dialect|
          +--------+-------+
                   |
                   v
          +------------------------+
          | canonicalization passes|
          +-----------+------------+
                      |
                      v
          +------------------------+
          | arith / func / scf     |
          +-----------+------------+
                      |
                      v
          +------------------------+
          | LLVM dialect           |
          +-----------+------------+
                      |
                      v
          +------------------------+
          | LLVM IR                |
          +-----------+------------+
                      |
                      v
          +------------------------+
          | Target/RiscV backend   |
          | llc, riscv64 triple    |
          +-----------+------------+
                      |
                      v
          +------------------------+
          | RISC-V asm / object    |
          +------------------------+
```

## 3. Source Language v0

The first language should be just powerful enough to prove the compiler
pipeline.

### Supported Syntax

```zc
func main() -> i32 {
  let x = 1 + 2 * 3;
  return x;
}
```

### v0 Features

- `func` function declaration.
- `main` entry point.
- `i32` integer type.
- Integer literals.
- Local immutable variables with `let`.
- Binary arithmetic: `+`, `-`, `*`, `/`.
- `return` statement.

### Later Language Features

- Function parameters.
- Function calls.
- `if` / `else`.
- `while` or `for`.

Phase 17 starts the next language layer. Its design is recorded in
[docs/phase17-functions-memory.md](docs/phase17-functions-memory.md): function
parameters, function calls, straight-line assignment, and the first planned
memory-model direction.
- Arrays and memory references.
- Vector values.
- Builtin operations that map naturally to RVV.

## 4. Frontend Components

### Lexer

The lexer converts source text into tokens.

Initial token kinds:

- Keywords: `func`, `let`, `return`.
- Type names: `i32`.
- Identifiers.
- Integer literals.
- Operators: `+`, `-`, `*`, `/`, `=`, `->`.
- Delimiters: `(`, `)`, `{`, `}`, `;`.

### Parser

The parser builds an AST from tokens.

Important parser rules:

- Parse function declarations.
- Parse blocks.
- Parse `let` statements.
- Parse `return` statements.
- Parse arithmetic expressions with operator precedence.

### AST

The AST should stay frontend-specific and independent from MLIR.

Suggested nodes:

- `ModuleAST`
- `FunctionAST`
- `BlockAST`
- `LetStmtAST`
- `ReturnStmtAST`
- `ExprAST`
- `IntegerLiteralAST`
- `VariableExprAST`
- `BinaryExprAST`

## 5. MLIR Layer

The compiler should start with a custom `zc` dialect, then lower into existing
MLIR dialects.

### zc Dialect v0

Initial operations:

- `zc.func`
- `zc.return`
- `zc.constant`
- `zc.add`
- `zc.sub`
- `zc.mul`
- `zc.div`
- `zc.let` or direct SSA value binding during emission

For the first version, local variables can be represented as a symbol table in
the MLIR generator and mapped directly to SSA values. A dedicated `zc.let`
operation is optional.

### Lowering Strategy

The early lowering path:

```text
zc dialect
  -> func dialect
  -> arith dialect
  -> LLVM dialect
  -> LLVM IR
```

Mapping examples:

- `zc.func` -> `func.func`
- `zc.return` -> `func.return`
- `zc.constant` -> `arith.constant`
- `zc.add` -> `arith.addi`
- `zc.sub` -> `arith.subi`
- `zc.mul` -> `arith.muli`
- `zc.div` -> `arith.divsi`

## 6. Command-Line Tools

The first executable can be named `zc`.

Suggested commands:

```bash
zc input.zc --emit-ast
zc input.zc --emit-mlir
zc input.zc --emit-llvm
zc input.zc --emit-riscv-asm -o output.s
```

For early development, the compiler may emit MLIR first and use external MLIR
and LLVM tools for the remaining stages:

```bash
zc input.zc --emit-mlir -o output.mlir
mlir-opt output.mlir ...
mlir-translate ...
llc -mtriple=riscv64-linux-gnu ...
```

Later, these stages can be integrated into the `zc` driver.

## 7. Project Layout

Recommended repository structure:

```text
zcompiler/
  README.md
  arch.md
  plan.md
  CMakeLists.txt
  include/
    zcompiler/
      AST/
      Parser/
      MLIRGen/
      Target/RiscV/
      Dialect/ZC/
      Conversion/
  lib/
    AST/
    Parser/
    MLIRGen/
    Target/RiscV/
    Dialect/ZC/
    Conversion/
  tools/
    zc/
      zc.cpp
  test/
    parser/
    mlir/
    lowering/
    riscv/
  examples/
    hello.zc
  profiles/
    rvv-default.json
```

## 8. Testing Strategy

Every stage should have small tests.

- Lexer tests: source text to token stream.
- Parser tests: source text to AST.
- MLIR emission tests: AST to `zc` dialect.
- Lowering tests: `zc` dialect to standard MLIR / LLVM dialect.
- RISC-V tests: generated assembly diff plus assembler validation when
  `riscv64-linux-gnu-as` is available.
- End-to-end tests: compile `main` and run with QEMU when possible.

## 9. RVV Direction

The toy compiler should not implement RVV immediately. Instead, it should keep
the architecture ready for future vector work.

Future RVV path:

```text
zc vector language feature
  -> zc vector ops
  -> MLIR vector dialect
  -> accelerator profile policy
  -> LLVM vector / RVV intrinsics
  -> RISC-V RVV assembly
```

Potential future optimizations:

- Vectorization hints.
- Loop canonicalization.
- Loop tiling.
- Memory access analysis.
- RVV intrinsic selection.
- AI-assisted pass exploration and performance tuning.

## 10. Roadmap To The Final Goal

The final goal is:

```text
an AI self made compiler based on RISCV RVV accelerator
```

The current roadmap status:

- Phase 12-16: completed the registered dialect, lowering pass, MLIR API
  emission, LLVM IR pipeline, and RISC-V backend path.
- Phase 17: completed functions, calls, assignment, and scalar pointer
  load/store.
- Phase 18: completed target-independent `vector_add` syntax and AST.
- Phase 19: completed masked MLIR vector dialect lowering for
  `vector<4xi32>` chunks.
- Phase 20: completed direct RVV reference assembly and formal MLIR/LLVM RVV
  lowering probe for vector add.
- Phase 21: completed reproducible vector-add artifact workflow with scalar
  baseline metadata.
- Phase 22: completed first AI-assisted experiment record.
- Phase 23: completed the first machine-readable RVV accelerator profile.
- Phase 24: completed host-side correctness tests for masked vector-add tails.
- Phase 25: expanded the vector kernel surface with `vector_copy`,
  `vector_scale`, and `vector_reduce_add`.

The next implementation priority is moving toward formal MLIR-to-RVV lowering
and emulator-backed correctness tests when a suitable RISC-V environment is
available.

## Phase 30R Architecture Update

Masked store adds a memory-side-effect mask consumer while preserving the frontend/backend boundary:

```text
vector_mask_* statement
  -> transient function-local VectorMaskStmtAST
  -> vector_masked_store statement
  -> VectorMaskedStoreStmtAST
  -> MLIR tail mask AND compare mask
  -> vector.transfer_write with combined mask
  -> RVV v0 compare mask
  -> predicated vse32.v ..., v0.t
```

This keeps mask production target independent in the AST and makes the RISC-V backend solely responsible for choosing the RVV predicated store instruction.

## Phase 30S Architecture Update

Masked load complements masked store, but it must not rely on RVV masked-off destination lane preservation while the profile uses `ta, ma`:

```text
vector_masked_load out, input, m0, passthrough, n
  -> VectorMaskedLoadStmtAST
  -> MLIR masked transfer_read + arith.select passthrough
  -> RVV masked vle32.v ..., v0.t
  -> RVV vmerge.vvm passthrough
  -> RVV vse32.v
```
