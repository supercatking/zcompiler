# Phase 34A/34B: Strided and Indexed Loads

## Goal

Add the first non-unit-stride memory forms needed for a broader RVV 1.0 surface.

## Syntax

```zc
vector_strided_load out, input, stride, n;
vector_indexed_load out, input, indices, n;
```

- `vector_strided_load` treats `stride` as an element stride and emits
  `vlse32.v`.
- `vector_indexed_load` treats `indices` as `i32` element indices, shifts them to
  byte offsets, and emits `vluxei32.v`.

## Result

Both forms have lexer/parser/AST/codegen goldens and QEMU validation in
`test/qemu/vector_memory_mask_widen_harness.c`.
