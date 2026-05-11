# Accelerator Profile

The accelerator profile records the target assumptions that affect lowering,
benchmark generation, and AI-assisted optimization analysis.

The first profile is:

```text
profiles/rvv-default.json
```

## Why This Exists

Compiler results are only meaningful when the target assumptions are explicit.
For the current RVV work, the important assumptions include:

- RISC-V target triple.
- `march` and `mabi`.
- RVV element width and LMUL policy.
- Tail and mask policy.
- Whether the compiler is using the direct RVV reference backend or the formal
  MLIR/LLVM path.
- Toolchain versions that generated the artifacts.

## Current Profile Summary

`rvv-default.json` describes the current development target:

- `riscv64-unknown-elf`
- `rv64gcv`
- `lp64d`
- default vector element type: `i32`
- current source operations: `vector_add`, `vector_copy`, `vector_scale`
- default MLIR vector type: `vector<4xi32>`
- tail handling: `vector.create_mask` plus masked transfer ops
- current backend: direct RVV reference assembly
- formal MLIR/LLVM RVV path: blocked at RISC-V `llc` toolchain mismatch

## Usage

Benchmarks should include the profile path in their generated metadata:

```json
"accelerator_profile": "profiles/rvv-default.json"
```

AI experiment records should also cite the profile when comparing optimization
results. This keeps future performance or code-size claims tied to a concrete
target assumption instead of an implicit machine model.

## Phase 25B Update

`vector_scale` is now part of the default profile. It keeps the same `i32`,
`vector<4xi32>`, and masked-tail assumptions as `vector_add` and `vector_copy`,
but adds scalar-to-vector broadcast plus multiply semantics:

```text
vector_scale c, a, factor, n;
```

The direct RVV reference backend maps this to `vle32.v`, `vmul.vx`, and
`vse32.v` inside the same `vsetvli` loop policy.
