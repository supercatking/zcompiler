# Implementation Plan

This document defines the phased implementation plan for the first `zcompiler`
toy compiler.

## Phase 0: Repository Bootstrap

Goal: create a clean GitHub project foundation.

Deliverables:

- `README.md` with final project vision.
- `arch.md` with toy compiler architecture.
- `plan.md` with phased goals.
- `.gitignore`.
- Basic directory skeleton.

Exit criteria:

- Repository can be cloned on WSL.
- Documentation explains the first compiler direction clearly.
- GitHub `main` branch has the initial project files.

## Phase 1: Build System and Tool Skeleton

Goal: build a minimal command-line compiler executable.

Deliverables:

- Top-level `CMakeLists.txt`.
- `tools/zc/zc.cpp`.
- Basic command-line options:
  - `--emit-tokens`
  - `--emit-ast`
  - `--emit-mlir`
  - `-o <file>`
- Initial CI-friendly build instructions.

Exit criteria:

- `cmake -G Ninja -S . -B build` works.
- `cmake --build build` produces `zc`.
- `zc --help` runs successfully.

## Phase 2: Lexer

Goal: convert `.zc` source text into tokens.

Deliverables:

- Token definitions.
- Lexer implementation.
- Error reporting with line and column.
- Token dump mode through `zc --emit-tokens`.
- Lexer tests.

Exit criteria:

- The compiler can tokenize the first example:

```zc
func main() -> i32 {
  let x = 1 + 2 * 3;
  return x;
}
```

## Phase 3: Parser and AST

Goal: parse tokens into a structured AST.

Deliverables:

- AST node classes.
- Recursive descent parser.
- Operator precedence parser for arithmetic expressions.
- AST dump mode through `zc --emit-ast`.
- Parser tests.

Exit criteria:

- Valid source files produce readable AST output.
- Invalid syntax produces useful errors.

## Phase 4: First MLIR Emission

Goal: emit valid MLIR from the AST.

Deliverables:

- MLIR context setup.
- First MLIR generator.
- Initial use of builtin, func, and arith dialects.
- `zc --emit-mlir`.
- MLIR output tests.

Exit criteria:

- Example source emits MLIR similar to:

```mlir
module {
  func.func @main() -> i32 {
    %c1 = arith.constant 1 : i32
    %c2 = arith.constant 2 : i32
    %c3 = arith.constant 3 : i32
    %0 = arith.muli %c2, %c3 : i32
    %1 = arith.addi %c1, %0 : i32
    return %1 : i32
  }
}
```

## Phase 5: Custom zc Dialect

Goal: introduce a project-owned MLIR dialect.

Deliverables:

- `zc` dialect definition.
- TableGen operation definitions.
- CMake integration for MLIR TableGen.
- Operations for constants, arithmetic, function, and return.
- Tests for parsing and printing `zc` dialect MLIR.

Exit criteria:

- The compiler can emit valid `zc` dialect MLIR.
- `mlir-opt` can parse and print the generated dialect when linked with the
  project plugin or tool.

## Phase 6: Lowering Passes

Goal: lower from `zc` dialect to standard MLIR dialects.

Deliverables:

- `zc` to `func` / `arith` conversion pass.
- Canonicalization patterns.
- Pass pipeline option in `zc`.
- Lowering tests.

Exit criteria:

- `zc` dialect MLIR lowers to standard MLIR.
- The lowered MLIR no longer contains `zc` operations.

## Phase 7: LLVM IR Emission

Goal: lower standard MLIR to LLVM dialect and LLVM IR.

Deliverables:

- MLIR lowering pipeline to LLVM dialect.
- `mlir-translate` compatible output.
- `zc --emit-llvm`.
- LLVM IR tests.

Exit criteria:

- Example source compiles to LLVM IR.
- The generated LLVM IR has a valid `main` returning `i32`.

## Phase 8: RISC-V Assembly

Goal: generate RISC-V assembly using LLVM backend.

Deliverables:

- `zc --emit-riscv-asm`.
- Integration with LLVM target settings.
- Initial `riscv64-linux-gnu` target triple support.
- Assembly tests.

Exit criteria:

- Example source generates RISC-V assembly.
- Return value can be validated through QEMU or assembly inspection.

## Phase 9: Control Flow

Goal: add basic control flow to prepare for real programs.

Deliverables:

- `if` / `else`.
- `while` or `for`.
- Comparison operators.
- Lowering to `scf` and then LLVM.

Exit criteria:

- Small branching and loop programs compile and run.

## Phase 10: Vector and RVV Preparation

Goal: prepare the compiler for RISC-V Vector Extension work.

Deliverables:

- Vector type syntax proposal.
- Initial vector operation design.
- Lowering path through MLIR vector dialect.
- RVV research notes and examples.

Exit criteria:

- The architecture has a concrete path from source-level vector operations to
  MLIR vector dialect and then to RISC-V RVV code generation.

## Phase 11: AI-Assisted Compiler Direction

Goal: begin connecting compiler development with AI-assisted workflows.

Deliverables:

- Pass tuning experiment format.
- Benchmark collection format.
- Prompt and evaluation notes for AI-generated optimization suggestions.
- Safety rules for accepting AI-generated compiler changes.

Exit criteria:

- The project can record optimization experiments and compare generated code or
  runtime results.

## Immediate Next Tasks

The next implementation step after these documents:

1. Add `.gitignore`.
2. Add CMake skeleton.
3. Add `tools/zc/zc.cpp`.
4. Implement `zc --help`.
5. Add the first example program in `examples/hello.zc`.

