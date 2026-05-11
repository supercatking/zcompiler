# Accelerator Profiles

This directory contains machine-readable target accelerator assumptions used by
benchmarks, lowering experiments, and AI-assisted optimization records.

Current profiles:

- `rvv-default.json`: the default RVV 1.0 development profile for the current
  `i32`, `LMUL=m1`, unit-stride vector-kernel subset.

Profiles are not hardware measurements. They describe the compiler policy and
toolchain assumptions that generated artifacts depend on.
