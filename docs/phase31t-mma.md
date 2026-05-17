# Phase 31T: MMA / Matrix Multiply v1

## Goal

Add the first compiler-owned matrix multiply operation instead of relying on
hand-written nested loops in the source language.

## Source Syntax

```zc
matrix_multiply c, a, b, rows, cols, inner;
```

Semantics for v1:

- `a` is a row-major `i32` matrix with shape `rows x inner`.
- `b` is a row-major `i32` matrix with shape `inner x cols`.
- `c` is a row-major `i32` output matrix with shape `rows x cols`.
- The operation overwrites `c[i * cols + j]` with the wrapping `i32` dot product
  of row `i` from `a` and column `j` from `b`.
- `rows`, `cols`, and `inner` are expected to be non-negative `i32` dimensions.

## Architecture

The operation is represented as a dedicated `MatrixMultiplyStmtAST` node. This
keeps matrix semantics separate from the existing one-dimensional vector kernel
surface and gives future phases a stable hook for RVV tiling, transposed-B
packing, and accelerator-specific lowering.

Current lowering paths:

- Lexer/parser/AST dump support for `matrix_multiply`.
- MLIR API generation to nested `scf.for` loops with `memref.load`, `arith.muli`,
  `arith.addi`, and `memref.store`.
- Direct RISC-V assembly generation to a scalar, ABI-safe nested loop. The
  direct backend saves and restores `s0`-`s11` before using them as loop state.
- QEMU runtime validation through a C harness with multiple matrix shapes.


Manual visible QEMU demo for `matrix_multiply`:

```bash
cd /home/zyz/zcompiler
./build/tools/zc/zc examples/matrix_multiply.zc --emit-riscv-asm > /tmp/matrix_multiply.s
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d /tmp/matrix_multiply.s test/qemu/matrix_multiply_harness.c -o /tmp/matrix_multiply
/home/qemu/qemu/build-riscv64-user/qemu-riscv64 -cpu max /tmp/matrix_multiply
# matrix_multiply demo 2x3 * 3x2 = [58 64; 139 154]
```

## Current Boundary

This phase is correct scalar MMA support, not RVV-optimized matrix multiply by
itself. Phase 31U builds on this node with `matrix_multiply_packed_b`, where the
right-hand matrix is column-packed so direct RVV dot-product lowering can use
unit-stride vector loads. Remaining optimization paths include compiler-owned
B packing, reusable dot-product lowering, strided/indexed alternatives, and tiled
matmul policy in the accelerator profile.
