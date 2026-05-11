# Phase 26A RVV Toolchain Alignment

Phase 26A turns the formal MLIR-to-RVV blocker into a repeatable toolchain
diagnostic.

## Problem

The compiler can already emit target-independent masked MLIR vector IR and can
lower that IR to LLVM dialect, LLVM IR, and bitcode. The remaining blocker is
the final `llc` step for RISC-V RVV assembly:

- `/home/zyz/mlir/build/bin/llc` is LLVM 23, matching the local MLIR tools, but
  it was configured with `LLVM_TARGETS_TO_BUILD=host`, so it only registers x86.
- `/usr/bin/llc` registers RISC-V, but it is LLVM 14 and cannot consume LLVM 23
  IR/bitcode.

## Diagnostic Script

Run:

```bash
./scripts/check-rvv-toolchain.sh
```

The script writes:

```text
build/experiments/rvv-toolchain/rvv-toolchain-diagnostic.json
build/experiments/rvv-toolchain/rvv-toolchain-diagnostic.md
```

The JSON records:

- MLIR tool versions.
- Local `llc` version and registered RISC-V target status.
- System `llc` version and registered RISC-V target status.
- RISC-V assembler and objdump presence.
- Relevant CMake cache values from the local MLIR build.
- Overall readiness status.

## Current Expected Status

```text
blocked_local_llc_missing_riscv_target
```

This status means the correct next action is not a compiler frontend change.
The local LLVM/MLIR toolchain needs a same-version `llc` with RISC-V enabled.

Current local diagnosis:

```text
MLIR major version: 23
Local llc: LLVM 23, missing RISC-V target
System llc: LLVM 14, has RISC-V target, version mismatch
LLVM_TARGETS_TO_BUILD: host
```

## Recommended Next Build

Use a separate build directory so the current `/home/zyz/mlir/build` remains
untouched:

```bash
cmake -G Ninja -S /home/zyz/mlir/llvm-project/llvm \
  -B /home/zyz/mlir/build-riscv \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_TARGETS_TO_BUILD="X86;RISCV" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build /home/zyz/mlir/build-riscv --target llc mlir-opt mlir-translate llvm-as
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/check-rvv-toolchain.sh
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/probe-formal-rvv-lowering.sh
```

Only after this diagnostic reports a same-version RISC-V-capable `llc` should
the compiler prefer the formal MLIR/LLVM path over the current direct RVV
reference backend.
