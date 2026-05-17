# Phase 31U: Packed-B RVV Matrix Multiply

## Goal

Move matrix multiply from scalar row-by-column memory access toward an RVV-friendly
layout where each dot product reads both operands with unit-stride vector loads.

## Source Syntax

```zc
matrix_multiply_packed_b c, a, packed_b, rows, cols, inner;
```

Semantics:

- `a` is a row-major `i32` matrix with shape `rows x inner`.
- `packed_b` is the column-packed form of original `B`, with shape
  `cols x inner`.
- `packed_b[col * inner + k]` must equal original `B[k * cols + col]`.
- `c` is a row-major `i32` output matrix with shape `rows x cols`.
- The operation overwrites `c[row * cols + col]` with the wrapping `i32` dot
  product of `a[row, :]` and original `B[:, col]`.
- `rows`, `cols`, and `inner` are expected to be non-negative `i32` dimensions.

## Architecture

Phase 31U reuses `MatrixMultiplyStmtAST` and adds an explicit RHS layout enum:

```text
row_major       -> matrix_multiply
packed_columns  -> matrix_multiply_packed_b
```

This keeps the matrix operation as one compiler-owned semantic node while making
the memory layout visible to MLIR generation and target lowering. The direct
RISC-V backend now has two paths:

- `matrix_multiply`: scalar row-major reference lowering.
- `matrix_multiply_packed_b`: RVV dot-product lowering using unit-stride
  `vle32.v`, `vmul.vv`, and `vredsum.vs` chunks for each output element.

## Validation

Validated paths include token dump, AST dump, MLIR generation, `mlir-opt`, direct
RVV assembly, assembler/objdump checks, and QEMU runtime correctness.

Manual visible QEMU demo:

```bash
cd /home/zyz/zcompiler
./build/tools/zc/zc examples/matrix_multiply_packed_b.zc --emit-riscv-asm > /tmp/matrix_multiply_packed_b.s
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d /tmp/matrix_multiply_packed_b.s test/qemu/matrix_multiply_packed_b_harness.c -o /tmp/matrix_multiply_packed_b
/home/qemu/qemu/build-riscv64-user/qemu-riscv64 -cpu max /tmp/matrix_multiply_packed_b
# matrix_multiply_packed_b demo 2x3 * 3x2 = [58 64; 139 154]
```

## Current Boundary

This is the first RVV matrix multiply path, but it still requires the caller or
a future compiler phase to provide `packed_b`. Next phases should add a compiler
owned pack/transpose operation, reusable dot-product lowering, and tile policy.
