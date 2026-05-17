# Phase 39C: Masked Non-Unit RVV Memory

## Goal

Phase 39C completes the current `i32` non-unit memory slice by adding masked
strided and indexed load/store forms on top of the existing transient mask
architecture.

## Source Syntax

```zc
vector_masked_strided_load out, input, stride, m0, passthrough, n;
vector_masked_indexed_load out, input, indices, m0, passthrough, n;
vector_masked_strided_store base, values, stride, m0, n;
vector_masked_indexed_store base, values, indices, m0, n;
```

`stride` and `indices` are measured in source elements, not bytes. The current
slice is limited to `ptr<i32>` buffers and LMUL `m1`.

## Semantics

- `vector_masked_strided_load`: for each `i < n`, writes
  `input[i * stride]` when `m0[i]` is true, otherwise writes
  `passthrough[i]`.
- `vector_masked_indexed_load`: for each `i < n`, writes `input[indices[i]]`
  when `m0[i]` is true, otherwise writes `passthrough[i]`.
- `vector_masked_strided_store`: for each `i < n`, writes
  `base[i * stride] = values[i]` only when `m0[i]` is true.
- `vector_masked_indexed_store`: for each `i < n`, writes
  `base[indices[i]] = values[i]` only when `m0[i]` is true.

False lanes in stores preserve the existing destination memory. Load tails beyond
`n` preserve the existing output memory because the loop never writes those
lanes.

## Direct RVV Lowering

- `vector_masked_strided_load` emits `vlse32.v ..., v0.t`, then merges with
  passthrough via `vmerge.vvm` before `vse32.v`.
- `vector_masked_indexed_load` loads i32 element indices, shifts them to byte
  offsets, emits `vluxei32.v ..., v0.t`, then merges with passthrough.
- `vector_masked_strided_store` emits `vsse32.v ..., v0.t`.
- `vector_masked_indexed_store` loads i32 element indices, shifts them to byte
  offsets, and emits `vsuxei32.v ..., v0.t`.

The mask operand may be produced by `vector_mask_*` or by logical mask
composition. The direct backend resolves it into `v0`.

## Validation

Each operation has lexer, parser, and RISC-V assembly goldens. Codegen checks
look for the expected RVV instruction families:

- `vlse32.v`
- `vluxei32.v`
- `vsse32.v`
- `vsuxei32.v`
- `vmerge.vvm`
- `v0.t`

The QEMU harness covers lengths `0, 1, 2, 3, 5, 8, 17, 31` and checks false-lane
and tail preservation for stores and masked loads.

## Remaining Gaps

Phase 39C does not add segment memory, fault-only-first memory, whole-register
load/store, `i8/i16/i64` non-unit memory, or the `tu/mu` policy matrix. Those
remain part of the broader RVV 1.0 roadmap.
