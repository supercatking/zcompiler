# Phase 30B: QEMU Execution Manifest

## Goal

Move the QEMU execution matrix out of ad hoc shell/C literals and into a small
machine-readable manifest. This is the first step toward generated QEMU
correctness harnesses for future RVV kernels.

## Manifest

```text
test/qemu/rvv_execution_manifest.json
```

The manifest records:

- `qemu_cpu`: CPU mode passed to `qemu-riscv64 -cpu`.
- `print_i32`: source program plus expected stdout and process exit status.
- `rvv_execution.sources`: zcompiler source files compiled into the executable.
- `rvv_execution.validated_kernels`: kernel families covered by the harness.
- `rvv_execution.lengths`: tail and multi-iteration length matrix.

## Harness Flow

```text
rvv_execution_manifest.json
  -> test/qemu/run.sh
  -> temporary qemu_manifest.env
  -> generated C harness length initializer
  -> static RISC-V64 Linux ELF
  -> qemu-riscv64 -cpu <manifest qemu_cpu>
```

## Current Limit

The C correctness logic is still written in `test/qemu/run.sh`. A later phase
should generate per-kernel C checks from structured kernel descriptors so adding
a new kernel requires only a source example, manifest entry, and expected
semantics model.

The current script derives the temporary C array capacity from the largest manifest length.
