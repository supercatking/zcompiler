# Phase 30G: QEMU Harness Generator

## Goal

Move the RVV runtime C harness out of `test/qemu/run.sh` and generate it from
the validated QEMU manifest. This keeps the shell script focused on orchestration
and gives kernel descriptors a concrete generation path.

## Implementation

Phase 30G adds:

```text
test/qemu/harness.py
```

The generator imports `load_and_validate` from `test/qemu/manifest.py`, reads
`test/qemu/rvv_execution_manifest.json`, and emits the temporary C harness used
by the QEMU test. The generated file includes comments derived from
`rvv_execution.kernel_checks`, the length matrix from the manifest, and the
array capacity derived from the largest tested length.

## Current Boundary

The generator still contains operation-specific C check templates for the current
kernel set. The next natural refinement is to move per-kernel check templates
into structured descriptors or small generator functions.
