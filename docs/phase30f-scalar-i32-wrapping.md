# Phase 30F: Scalar i32 Wrapping Semantics

## Goal

Align scalar source `i32` arithmetic with the wrapping bit-pattern policy already
validated for the current RVV kernel subset.

## Policy

Scalar source `i32` arithmetic uses 32-bit two's-complement wrapping for the
current compiler surface. In the direct RISC-V text backend, scalar arithmetic
lowers to RV64 word operations:

- `+` -> `addw`
- `-` -> `subw`
- `*` -> `mulw`
- `/` -> `divw`

Equality and inequality compare the 32-bit subtraction bit pattern before
`seqz`/`snez`. Relational comparisons continue to use signed comparisons over
sign-extended `i32` values.

## Validation Program

```text
examples/scalar_i32_wrap.zc
```

The program prints:

```text
-2147483648
2147483647
0
```

These values validate `2147483647 + 1`, `-2147483648 - 1`, and
`1073741824 * 4` under wrapping `i32` semantics.

## Boundary

The LLVM IR path already emits plain `i32` arithmetic without `nsw`/`nuw` flags.
This phase makes the direct RISC-V runtime path match that bit-width policy.
