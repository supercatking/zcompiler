# Phase 36A: Signed Widening Add Slice

## Goal

Add the first mixed-width arithmetic operation with explicit source and result
widths.

## Syntax

```zc
vector_widen_add_i16_i32 out, a, b, n;
```

`a` and `b` must be `ptr<i16>`. `out` must be `ptr<i32>`.

## Lowering

The direct RVV backend emits `vle16.v`, `vwadd.vv`, and `vse32.v` under an
`e16,m1,ta,ma` loop.

## Result

`examples/vector_widen_add_i16_i32.zc` is validated by lexer/parser/codegen
checks and by QEMU runtime execution.
