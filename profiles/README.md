# Accelerator Profiles

This directory contains machine-readable target accelerator assumptions used by
benchmarks, lowering experiments, and AI-assisted optimization records.

Current profiles:

- `rvv-default.json`: the first RISC-V RVV profile for `vector_add` work.

Profiles are not hardware measurements. They describe the compiler policy and
toolchain assumptions that generated artifacts depend on.
