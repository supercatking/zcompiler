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
- RVV spec version.
- Tail and mask policy.
- QEMU execution validation coverage.
- Whether the compiler is using the direct RVV reference backend or the formal
  MLIR/LLVM path.
- Toolchain versions that generated the artifacts.

## Current Profile Summary

`rvv-default.json` describes the current development target:

- `riscv64-unknown-elf`
- `rv64gcv`
- `lp64d`
- RVV spec version: `1.0`
- default vector element type: `i32`
- current compiler vector element width: `32`
- current LMUL support: `m1`
- current source operations: `vector_add`, `vector_copy`, `vector_scale`,
  `vector_mul`, `vector_reduce_add`
- default MLIR vector type: `vector<4xi32>`
- tail handling: `vector.create_mask` plus masked transfer ops
- current backend: direct RVV reference assembly
- current execution validation: `qemu-riscv64` CTest over lengths `0`, `1`,
  `2`, `3`, `4`, `5`, `7`, `8`, `9`, `16`, and `17`
- formal MLIR/LLVM RVV path: blocked at RISC-V `llc` toolchain mismatch

## RVV 1.0 Compliance Tracking

The profile is intentionally explicit that the current compiler is an RVV 1.0
compatible subset rather than a full RVV 1.0 implementation. The detailed
support matrix and acceptance rule live in:

```text
docs/rvv-1.0-compliance.md
```

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

## Phase 25C Update

`vector_reduce_add` is now part of the default profile. It is the first
memory-to-scalar vector kernel:

```text
vector_reduce_add sum, a, n;
```

The MLIR path uses a loop-carried scalar accumulator and `vector.reduction
<add>`. The direct RVV reference backend maps each chunk to `vle32.v`,
`vmv.s.x`, `vredsum.vs`, and `vmv.x.s`.

## Phase 30A Update

`vector_mul` is now part of the default profile. It keeps the same unit-stride
`i32`, `vector<4xi32>`, `m1`, and masked-tail assumptions as `vector_add`, but
uses elementwise vector-vector multiply semantics:

```text
vector_mul c, a, b, n;
```

The MLIR path lowers to masked `vector.transfer_read`, `arith.muli`, and
`vector.transfer_write`. The direct RVV reference backend maps this to
`vle32.v`, `vmul.vv`, and `vse32.v` inside the standard `vsetvli` loop policy.
