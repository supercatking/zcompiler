# Phase 40B2: i8/i64 Unit-Stride Reduction

## Goal

Phase 40B2 adds typed `vector_reduce_add` coverage for `i8` and `i64` while
making the accumulator/result-width rule explicit.

## Source Surface

```zc
vector_reduce_add result, input, n;
```

New examples:

- `examples/vector_reduce_add_i8.zc`
- `examples/vector_reduce_add_i64.zc`

## Accumulator Policy

This phase implements RVV same-SEW reduction, not widening reduction:

- `ptr<i8>` uses `e8` reduction and wraps in 8-bit lanes. The extracted scalar
  is carried in the existing `i32` source scalar slot as the sign-extended
  8-bit result.
- `ptr<i64>` uses `e64` reduction and requires an `i64` accumulator/result
  variable.
- `ptr<i16>` and `ptr<i32>` keep the existing policy: same-SEW reduction into an
  `i32` source scalar.

Future widening reduction should use a dedicated operation name instead of
silently changing `vector_reduce_add` semantics.

## Direct RVV Lowering

- `ptr<i8>` emits `vsetvli ..., e8, m1, ta, ma`, `vle8.v`, `vmv.s.x`,
  `vredsum.vs`, and `vmv.x.s`.
- `ptr<i64>` emits `vsetvli ..., e64, m1, ta, ma`, `vle64.v`, `vmv.s.x`,
  `vredsum.vs`, and `vmv.x.s`.

## Validation

The lexer, parser, and RISC-V assembly golden tests include the new examples.
The QEMU harness covers lengths `0, 1, 2, 3, 5, 8, 17, 31`.

## Remaining Work

Unsigned reductions, widening reductions, min/max reductions, and floating-point
reductions remain separate roadmap items.
