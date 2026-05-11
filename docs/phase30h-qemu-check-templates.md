# Phase 30H: QEMU Check Templates

## Goal

Split the generated QEMU RVV harness into explicit generator units for each
runtime check family. This keeps the harness generator readable as the kernel
surface grows.

## Implementation

`test/qemu/harness.py` now has separate render functions for:

- manifest-derived kernel comments
- deterministic signed/wrapping seed helpers
- `complex_vector_pipeline` add/scale/reduce checks
- `copy_then_sum` copy/reduce checks
- `vmul` multiply checks

The generated C output remains behaviorally equivalent to Phase 30G, but the
Python generator now has clearer extension points for future compare/select or
element-width-specific checks.
