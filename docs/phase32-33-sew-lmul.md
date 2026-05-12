# Phases 32A-33A: RVV Compliance Matrix, SEW, and LMUL Slices

## Goal

Move from an implicit i32/m1-only prototype toward an explicit RVV 1.0 coverage
matrix and typed-buffer-driven vector code generation.

## Result

- Added `profiles/rvv-compliance-matrix.json` and
  `scripts/generate-rvv-compliance.py`.
- Added generated compliance table at
  `docs/rvv-1.0-compliance-generated.md`.
- Parser accepts `i8`, `i16`, `i32`, `i64` and matching `ptr<T>` buffer types.
- Direct RVV `vector_add` derives SEW from typed pointer arguments.
- Added `vector_add_m2` and `vector_add_m4` syntax. Phase 33A validates the
  `m2` path with QEMU; `m4` is parser/backend-ready but still needs a dedicated
  QEMU case before it is promoted from partial coverage.
