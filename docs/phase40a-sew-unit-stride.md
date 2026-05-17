# Phase 40A: i8/i64 Unit-Stride SEW Slice

## Goal

Phase 40A broadens the validated RVV SEW matrix beyond the existing `i16/i32`
subset. It focuses on unit-stride operations first, because those share a simple
addressing model and avoid mixing SEW expansion with indexed-memory EMUL policy.

## Implemented Source Surface

The following target-independent operations now have QEMU-validated `i8` and
`i64` examples:

```zc
vector_add c, a, b, n;
vector_copy out, input, n;
vector_select_gt out, lhs, rhs, true_values, false_values, n;
```

Example files:

- `examples/vector_add_i8.zc`
- `examples/vector_add_i64.zc`
- `examples/vector_copy_i8.zc`
- `examples/vector_copy_i64.zc`
- `examples/vector_select_i8_gt.zc`
- `examples/vector_select_i64_gt.zc`

## Direct RVV Lowering

`vector_add` already derived SEW from typed pointer operands. Phase 40A extends
the same policy to `vector_copy` and `vector_select`:

- `ptr<i8>` uses `vsetvli ..., e8, m1, ta, ma`, `vle8.v`, and `vse8.v`.
- `ptr<i64>` uses `vsetvli ..., e64, m1, ta, ma`, `vle64.v`, and `vse64.v`.
- `vector_select_gt` uses the selected SEW for all four input vectors and emits
  the normal RVV compare plus `vmerge.vvm`.

All participating buffers in a typed unit-stride operation must have matching
element widths. Mixed-width operations remain rejected unless a dedicated
widening/narrowing op documents the contract.

## Validation

Phase 40A adds lexer, parser, and RISC-V assembly goldens for each new example.
Codegen checks look for `e8`, `e64`, `vle8.v`, `vse8.v`, `vle64.v`, `vse64.v`,
and the expected compare/select instructions.

The QEMU harness covers lengths `0, 1, 2, 3, 5, 8, 17, 31` and checks that tail
elements remain unchanged.

## Remaining Work

This phase does not yet broaden every RVV instruction family to every SEW. The
main remaining SEW work is:

- `i8/i64` coverage for multiply, scale, reductions, and masked arithmetic.
- non-unit `i8/i16/i64` memory forms with explicit indexed-memory EMUL policy.
- full illegal-combination tests for unsupported mixed-width operands.
