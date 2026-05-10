# Toy Compiler Architecture

This document defines the first architecture of `zcompiler`, a small
MLIR-based toy compiler that will grow toward an AI-assisted RISC-V RVV compiler.

## Architecture Diagram

![zcompiler architecture](arch.svg)

The diagram shows the current project shape after the toy compiler phases:

- The frontend owns `.zc` source parsing: lexer, parser, and AST.
- The AST is the central in-memory program model used by later compiler stages.
- The `CodeGen` module emits several textual targets from the AST:
  standard MLIR, `zc` dialect MLIR surface, lowered MLIR, LLVM IR, and RISC-V
  assembly.
- The RVV and AI-assisted blocks are deliberate future-facing architecture
  areas. They are documented now so the toy compiler can grow toward the final
  accelerator-oriented goal without changing direction later.

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
      Dialect/ZC/
      Conversion/
  lib/
    AST/
    Parser/
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
```

## 8. Testing Strategy

Every stage should have small tests.

- Lexer tests: source text to token stream.
- Parser tests: source text to AST.
- MLIR emission tests: AST to `zc` dialect.
- Lowering tests: `zc` dialect to standard MLIR / LLVM dialect.
- RISC-V tests: generated assembly contains expected instructions.
- End-to-end tests: compile `main` and run with QEMU when possible.

## 9. RVV Direction

The toy compiler should not implement RVV immediately. Instead, it should keep
the architecture ready for future vector work.

Future RVV path:

```text
zc vector language feature
  -> zc vector ops
  -> MLIR vector dialect
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

The remaining roadmap is:

- Phase 12: turn `zc` into a registered MLIR dialect.
- Phase 13: implement real MLIR rewrite-pattern lowering.
- Phase 14: build MLIR modules in memory with MLIR C++ APIs.
- Phase 15: lower through MLIR LLVM dialect and emit LLVM IR through
  infrastructure rather than hand-written text.
- Phase 16: generate RISC-V assembly through LLVM's RISC-V backend.
- Phase 17: add functions, calls, assignment, and memory operations.
- Phase 18: add target-independent vector syntax and vector AST nodes.
- Phase 19: lower vector operations to MLIR vector dialect.
- Phase 20: lower vector operations toward RVV output.
- Phase 21: add accelerator profiles and reproducible benchmarks.
- Phase 22: implement the AI-assisted optimization experiment loop.

The next implementation priority is Phase 12 because it upgrades the project
from a text-emitting toy compiler toward a real MLIR compiler.
