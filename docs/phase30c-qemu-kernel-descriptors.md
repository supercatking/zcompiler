# Phase 30C: QEMU Kernel Descriptors

## Goal

Give the QEMU RVV manifest enough structure to describe which kernels are
validated and where each runtime check lives. This keeps the runtime validation
metadata close to the execution matrix and prepares the next step: generated C
checks from descriptors.

## Implementation

Phase 30C adds:

```text
test/qemu/manifest.py
```

The helper validates `test/qemu/rvv_execution_manifest.json` and emits a small
temporary shell environment consumed by `test/qemu/run.sh`. The manifest now has
`rvv_execution.kernel_checks`, with one descriptor per validated kernel. Each
descriptor records:

- `kernel`: source-level kernel family.
- `function`: runtime function or functions that exercise the kernel.
- `check`: the semantic condition checked by the C harness.

## Current Boundary

The descriptors are validated, but the C assertion bodies are still written by
hand in `test/qemu/run.sh`. The next phase should turn descriptor semantics into
generated C check fragments.
