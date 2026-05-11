# Phase 30D: Signed i32 Runtime Coverage

## Goal

Make current integer semantics explicit for RVV runtime tests and stop relying
only on positive input values.

## Current Policy

The current vector kernels operate on source-level `i32` values. Runtime QEMU
correctness tests now use mixed positive and negative inputs for:

- `vector_add`
- `vector_scale`
- `vector_copy`
- `vector_reduce_add`
- `vector_mul`

The C harness intentionally keeps expected values small enough to avoid signed C
overflow. That means Phase 30D validates signed no-overflow behavior. It does
not yet define a full source-language overflow contract.

## RVV Note

RVV integer add and multiply instructions operate on fixed-width element bits.
For the current compiler subset, emitted instructions are compatible with the
`i32` element representation used by the RISC-V C harness. A later phase should
choose and document whether source-level overflow is wrapping, trapping, or
undefined.

## Manifest

`test/qemu/rvv_execution_manifest.json` records the current integer semantics
policy under `rvv_execution.integer_semantics`, and `test/qemu/manifest.py`
validates that the policy is present.
