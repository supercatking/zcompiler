# Phase 18 Vector Syntax Design

Phase 18 introduces source-level vector concepts without tying the language to
RISC-V RVV instruction names.

## First Slice: `vector_add`

The first vector operation is a high-level buffer operation:

```zc
func vadd(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, n: i32) -> i32 {
  vector_add c, a, b, n;
  return 0;
}
```

Semantics:

- `a` and `b` are input `ptr<i32>` buffers.
- `c` is the output `ptr<i32>` buffer.
- `n` is the element count.
- The operation represents `c[i] = a[i] + b[i]` for `0 <= i < n`.

## Why This Syntax

- It is target-independent.
- It does not expose RVV instructions such as `vsetvli` or `vadd.vv` in source.
- It matches the scalar memory model from Phase 17B.
- It gives Phase 19 one clear operation to lower to MLIR vector dialect.

## AST Shape

`VectorAddStmtAST` stores:

- output buffer name
- left input buffer name
- right input buffer name
- length expression

## Lowering Plan

Phase 18 only parses and dumps the AST.

Phase 19 should lower `vector_add` to MLIR vector dialect, likely by expanding
to a vector loop shape that can later lower toward RVV:

```text
vector_add c, a, b, n
  -> vector loop / transfer_read / add / transfer_write
  -> LLVM vector or RVV-oriented lowering
```

## Test Plan

- Lexer golden test for `vector_add`.
- Parser AST golden test for `VectorAddStmt`.
- Phase 19 should replace the temporary rejection with MLIR vector dialect
  lowering.
