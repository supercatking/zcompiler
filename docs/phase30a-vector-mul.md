# Phase 30A: Vector Multiply Kernel

## Goal

Add a new target-independent vector arithmetic kernel:

```zc
vector_mul c, a, b, n;
```

The operation writes `c[i] = a[i] * b[i]` for `0 <= i < n` over `i32`
unit-stride buffers. It is part of the current RVV 1.0 compatible subset, not a
claim of full RVV 1.0 coverage.

## Architecture Boundary

`vector_mul` follows the existing vector-kernel layering:

- Lexer recognizes `vector_mul` as a source keyword.
- Parser builds a `VectorMulStmtAST` with output buffer, two input buffers, and
  length expression.
- AST remains target-independent and does not expose RVV instruction names.
- MLIRGen lowers to masked `vector.transfer_read`, `arith.muli`, and
  `vector.transfer_write` in an `scf.for` loop.
- Direct RISC-V output emits an RVV reference loop using `vsetvli`, `vle32.v`,
  `vmul.vv`, and `vse32.v`.

## MLIR Shape

```text
scf.for i = 0 to n step 4
  active = min(n - i, 4)
  mask = vector.create_mask active : vector<4xi1>
  lhs = vector.transfer_read a[i], mask : vector<4xi32>
  rhs = vector.transfer_read b[i], mask : vector<4xi32>
  product = arith.muli lhs, rhs : vector<4xi32>
  vector.transfer_write product, c[i], mask
```

## RVV Reference Shape

```asm
vsetvli vl, remaining, e32, m1, ta, ma
vle32.v v0, 0(lhs)
vle32.v v1, 0(rhs)
vmul.vv v2, v0, v1
vse32.v v2, 0(output)
```

## Validation

Phase 30A adds or updates:

- `examples/vector_mul.zc`
- lexer and parser golden tests
- MLIR golden output checked with `mlir-opt`
- RISC-V assembly golden output checked with assembler and objdump
- host correctness test for masked-tail semantics
- QEMU RVV execution harness coverage across the profile length matrix
- profile, RVV compliance, README, and progress documentation
