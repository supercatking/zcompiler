# Phase 40B1: i8/i64 Unit-Stride Mul/Scale

## Goal

Phase 40B1 continues the SEW expansion from Phase 40A by validating `i8` and
`i64` unit-stride multiply and scalar-scale operations.

## Implemented Source Surface

```zc
vector_mul c, a, b, n;
vector_scale out, input, factor, n;
```

New examples:

- `examples/vector_mul_i8.zc`
- `examples/vector_mul_i64.zc`
- `examples/vector_scale_i8.zc`
- `examples/vector_scale_i64.zc`

## Direct RVV Lowering

`vector_mul` and `vector_scale` now derive SEW from typed pointer operands:

- `ptr<i8>` emits `vsetvli ..., e8, m1, ta, ma`, `vle8.v`, `vmul.vv` or
  `vmul.vx`, and `vse8.v`.
- `ptr<i64>` emits `vsetvli ..., e64, m1, ta, ma`, `vle64.v`, `vmul.vv` or
  `vmul.vx`, and `vse64.v`.

All vector buffers in the operation must have matching element widths. Scale
uses the scalar source operand as the `vmul.vx` register value.

## Validation

The lexer, parser, and RISC-V assembly golden tests include all four new
examples. Codegen checks verify `e8`, `e64`, `vle8.v`, `vse8.v`, `vle64.v`,
`vse64.v`, `vmul.vv`, and `vmul.vx`.

The QEMU harness covers lengths `0, 1, 2, 3, 5, 8, 17, 31` and checks tail
preservation with sentinel values.

## Remaining Work

Reduction is intentionally left for a follow-up phase because accumulator width,
result type, and widening/narrowing policy need a clearer design than simple
elementwise operations.
