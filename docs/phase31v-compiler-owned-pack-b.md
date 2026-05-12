# Phase 31V: Compiler-Owned Pack-B

## Goal

Add a source-level packing operation so callers can pass row-major `B` and let
zcompiler prepare the column-packed layout required by the packed-B RVV matrix
multiply path.

## Syntax

```zc
matrix_pack_b packed_b, b, cols, inner;
matrix_multiply_packed_b c, a, packed_b, rows, cols, inner;
```

`matrix_pack_b` writes `packed_b[col * inner + k] = b[k * cols + col]`.

## Result

- Lexer/parser/AST support for `matrix_pack_b`.
- MLIR nested-loop lowering for the pack operation.
- Direct scalar RISC-V pack loop feeding the existing RVV packed-B dot product.
- QEMU validation through `test/qemu/matrix_pack_b_harness.c`.
