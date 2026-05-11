# Phase 26B RISCV LLVM Build Plan

Phase 26B adds a scriptable build plan for a same-version LLVM/MLIR toolchain
with the RISC-V target enabled.

## Why A Separate Build Directory

The current local build at `/home/zyz/mlir/build` is usable for MLIR
development, but it was configured with:

```text
LLVM_TARGETS_TO_BUILD=host
```

That means its `llc` cannot emit RISC-V. Phase 26B keeps the existing build
untouched and uses a separate directory:

```text
/home/zyz/mlir/build-riscv
```

## Script

Default dry run:

```bash
./scripts/prepare-riscv-llvm-build.sh
```

Configure only:

```bash
./scripts/prepare-riscv-llvm-build.sh --configure
```

Configure and build the required tools:

```bash
./scripts/prepare-riscv-llvm-build.sh --build
```

The script writes:

```text
build/experiments/rvv-toolchain/riscv-llvm-build-plan.json
build/experiments/rvv-toolchain/riscv-llvm-build-plan.md
```

## Default Build Plan

```bash
cmake -G Ninja -S /home/zyz/mlir/llvm-project/llvm \
  -B /home/zyz/mlir/build-riscv \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_TARGETS_TO_BUILD="X86;RISCV" \
  -DCMAKE_BUILD_TYPE=Release

cmake --build /home/zyz/mlir/build-riscv --target llc mlir-opt mlir-translate llvm-as
```

## Validation After Build

```bash
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/check-rvv-toolchain.sh
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/probe-formal-rvv-lowering.sh
```

If the diagnostic reports a same-version RISC-V-capable `llc`, the compiler can
start experimenting with preferring the formal MLIR/LLVM lowering path over the
direct RVV reference backend.
