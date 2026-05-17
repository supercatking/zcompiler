# Phase 28C: QEMU RVV Execution Validation

## Goal

Move from "the compiler emits RVV assembly" to "the emitted RISC-V64 binary
actually runs under QEMU and validates its result."

## QEMU Path

The default validation path is:

```text
/home/qemu/qemu/build-riscv64-user/qemu-riscv64
```

The test can be pointed at another binary with:

```bash
ZCOMPILER_QEMU_RISCV64=/path/to/qemu-riscv64 ctest --test-dir build -R qemu-riscv64
```

If `qemu-riscv64` or `riscv64-linux-gnu-gcc` is unavailable, the test prints a
skip message and exits successfully. This keeps the regular test suite usable
on machines that only have the compiler toolchain.

## Validation Cases

The `qemu-riscv64` CTest target validates two execution paths:

1. `examples/print_i32.zc`
   - compiles to RISC-V assembly
   - links to a static RISC-V64 Linux ELF
   - runs under QEMU
   - checks stdout is `220`
   - checks process exit status is `220`

2. `examples/complex_vector_pipeline.zc`
   - compiles zcompiler RVV assembly
   - links with a small C correctness harness
   - runs under QEMU with `-cpu max`
   - checks the vector add, scale, multiply, copy, and reduce results by exit
     status
   - covers lengths `0`, `1`, `2`, `3`, `4`, `5`, `7`, `8`, `9`, `16`,
     and `17`

## Execution Manifest

The QEMU test reads this manifest before generating the temporary C harness:

```text
test/qemu/rvv_execution_manifest.json
```

The manifest records the QEMU CPU mode, `print_i32` stdout/exit expectations,
the RVV source files used by the test, the validated kernel list, and the length
matrix. `test/qemu/run.sh` uses the length matrix to generate the C harness
initializer, so future length coverage changes are data updates instead of C
source edits.

## Manual Command

```bash
cd /home/zyz/zcompiler
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```
