# Phase 20C Formal MLIR/LLVM RVV Lowering Probe

Phase 20C investigates whether the target-independent masked vector IR can move
through the official MLIR and LLVM lowering stack toward RISC-V RVV assembly.

## Input Shape

The current `vector_add` frontend emits masked MLIR vector dialect IR:

```mlir
%active = arith.minui %remaining, %c4 : index
%mask = vector.create_mask %active : vector<4xi1>
%lhs = vector.transfer_read %arg0[%i], %c0_i32, %mask
    : memref<?xi32>, vector<4xi32>
%rhs = vector.transfer_read %arg1[%i], %c0_i32, %mask
    : memref<?xi32>, vector<4xi32>
%sum = arith.addi %lhs, %rhs : vector<4xi32>
vector.transfer_write %sum, %arg2[%i], %mask
    : vector<4xi32>, memref<?xi32>
```

This is a good compiler-internal shape because it is still target-independent
but preserves the tail mask needed by RVV-style code.

## Probe Command

```bash
./scripts/probe-formal-rvv-lowering.sh
```

The script writes artifacts to:

```text
build/experiments/mlir-rvv/
```

Key generated files:

- `vector_add.masked.mlir`
- `vector_add.vector-to-llvm.mlir`
- `vector_add.vector-to-llvm.ll`
- `vector_add.vector-to-llvm.bc`
- `formal-rvv-lowering-result.json`
- `formal-rvv-lowering-result.md`

## Pipeline Tested

```text
zc --emit-mlir
  -> mlir-opt --convert-vector-to-llvm --convert-scf-to-cf --convert-to-llvm
  -> mlir-translate --mlir-to-llvmir
  -> llvm-as
  -> llc -mtriple=riscv64-unknown-elf -mattr=+m,+v
```

## Findings

- MLIR vector lowering works for the current masked vector IR.
- The generated LLVM dialect lowers to LLVM IR successfully.
- The generated LLVM IR contains `llvm.masked.load` and `llvm.masked.store`.
- `/home/zyz/mlir/build/bin/llc` is LLVM 23 but currently has only x86 targets
  registered, so it cannot emit RISC-V.
- `/usr/bin/llc` is LLVM 14 and has RISC-V targets, but it cannot consume the
  LLVM 23 IR/bitcode output because of opaque pointer and attribute-version
  incompatibilities.

## Current Decision

Keep the direct RVV assembly backend as the reference output path while the
formal path is blocked by toolchain mismatch. The compiler architecture should
still keep generating masked MLIR vector IR because that is the correct input
for a future same-version LLVM/MLIR RVV backend.

## Next Actions

- Rebuild `/home/zyz/mlir/llvm-project` with RISC-V targets enabled, or install
  a same-version LLVM toolchain that includes RISC-V.
- Re-run `./scripts/probe-formal-rvv-lowering.sh`.
- Once `llc` produces assembly, inspect whether LLVM maps masked load/store and
  `<4 x i32>` arithmetic to RVV instructions or scalarized code.
- Only after the generated assembly is acceptable should `zc --emit-riscv-asm`
  start preferring this formal path for `vector_add`.
