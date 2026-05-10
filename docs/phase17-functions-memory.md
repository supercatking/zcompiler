# Phase 17 Functions and Memory Design

Phase 17 extends the toy source language enough to express simple kernel
building blocks without coupling the frontend directly to RVV or backend
details.

## Goals

- Add function parameters.
- Add function calls.
- Add straight-line assignment.
- Prepare a first memory model for later array and vector work.

## Non-Goals For The First Slice

- No heap allocation.
- No pointer arithmetic exposed in the source language.
- No RVV-specific source syntax.
- No vector load/store lowering in this slice.

## Source Syntax

Function parameters use explicit types:

```zc
func add(a: i32, b: i32) -> i32 {
  return a + b;
}
```

Function calls are expressions:

```zc
func main() -> i32 {
  let x = add(2, 3);
  return x;
}
```

Assignment updates the current local binding:

```zc
func main() -> i32 {
  let x = 1;
  x = x + 2;
  return x;
}
```

## AST Changes

- `ParameterAST`
  - `name`
  - `type`
- `CallExprAST`
  - `callee`
  - `arguments`
- `AssignStmtAST`
  - `name`
  - `value`
- `FunctionAST`
  - stores `std::vector<ParameterAST>`

The AST remains independent from MLIR headers.

## Lowering Strategy

For the first slice, assignment is represented as a new value bound to the same
source name. This keeps straight-line programs simple:

```text
let x = 1;    // x -> %0
x = x + 2;    // x -> %1
return x;     // uses %1
```

This is not a full mutable memory model yet. It is a deliberate stepping stone:
the parser and AST learn assignment now, while later array/memref work can add
explicit storage operations.

## MLIR Strategy

- Function parameters map to `func.func @name(%arg0: i32, ...) -> i32`.
- Calls map to `func.call @callee(...) : (...) -> i32`.
- Straight-line assignment updates the MLIRGen symbol table.

## LLVM/RISC-V Strategy

The infrastructure-backed path remains:

```text
AST
  -> in-memory MLIR
  -> MLIR LLVM dialect
  -> LLVM IR
  -> LLVM RISC-V backend assembly
```

The text LLVM/RISC-V emitters remain reference fallbacks for unsupported
features.

## Test Plan

- Lexer golden test for `:`, `,`, parameters, calls, and assignment.
- Parser AST golden test for a multi-function call program.
- MLIR golden test for `func.call`.
- LLVM IR validation with `llvm-as`.
- RISC-V assembly validation with `riscv64-linux-gnu-as` when available.

## Memory Model Next Step

After parameters, calls, and assignment are stable, add array-like syntax. The
Phase 17B implementation uses source-level `ptr<i32>` as the first buffer type:

```zc
func add_at(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, i: i32) -> i32 {
  let x = load a[i];
  let y = load b[i];
  let z = x + y;
  store c[i] = z;
  return z;
}
```

This slice introduces:

- `ptr<i32>` parameter types.
- `LoadExprAST` for `load buffer[index]`.
- `StoreStmtAST` for `store buffer[index] = value;`.

The source type name is intentionally `ptr<i32>` because the eventual LLVM/RVV
backend sees this as pointer-like data. The frontend still treats it as a typed
buffer abstraction rather than exposing pointer arithmetic to users.

## Phase 17B Lowering Choice

The initial implementation may lower through either of these paths:

```text
ptr<i32> source type
  -> memref-like MLIR buffer operation
  -> LLVM pointer representation
```

or:

```text
ptr<i32> source type
  -> direct LLVM pointer IR in the reference backend
  -> RISC-V scalar load/store
```

The preferred long-term path is the first one. The direct LLVM/RISC-V fallback
is acceptable only as a temporary reference path and must stay documented in
`progress.md`.
