# RVV 1.0 Compliance Roadmap

This document defines how zcompiler tracks compatibility with the RISC-V Vector
Extension 1.0 specification.

## Current Position

Current status:

```text
RVV 1.0 compatible subset: yes
Full RVV 1.0 compatibility: no
```

The compiler currently emits a small, executable RVV 1.0-shaped subset for
`i32` unit-stride kernels. The generated binaries are validated with
`qemu-riscv64 -cpu max`.

The active machine-readable profile is:

```text
profiles/rvv-default.json
```

Required profile fields for RVV 1.0 tracking:

- `rvv.spec_version`
- `rvv.minimum_extension`
- `rvv.required_base_isa`
- `rvv.supported_lmul`
- `rvv.tail_policy`
- `rvv.mask_policy`
- `current_execution_validation.tested_lengths`
- `current_compiler_policy.compliance_status`

## Current Supported Subset

| Area | Current support | Validation |
| --- | --- | --- |
| Base target | `rv64gcv`, `lp64d` | assembler, linker, QEMU |
| RVV spec target | RVV 1.0 profile field | JSON validation |
| SEW | validated unit-stride `i8/i16/i32/i64` slices for add/copy/mul/reduce/scale/select coverage, plus broader `i32` memory/mask coverage and `i16` widen coverage | QEMU correctness tests |
| LMUL | `m1`, plus `i16` `m2`/`m4` vector-add slices | assembly inspection and QEMU |
| Memory access | unit-stride `i32`/`i16` buffers plus strided/indexed `i32` loads/stores, including masked strided/indexed `i32` loads/stores | QEMU correctness tests |
| Vector length | dynamic `vsetvli` from remaining elements | tail-length QEMU tests |
| Tail/mask policy | `ta, ma` direct assembly policy | assembly inspection |
| Signed runtime inputs | mixed positive/negative `i32` including overflow bit patterns | QEMU |
| Add | `vector_add` -> `vadd.vv` | objdump and QEMU |
| Copy | `vector_copy` -> `vle32.v` + `vse32.v` | objdump and QEMU |
| Scale | `vector_scale` -> `vmul.vx` | objdump and QEMU |
| Multiply | `vector_mul` -> `vmul.vv` | objdump and QEMU |
| Reduction | `vector_reduce_add` -> `vredsum.vs` | objdump and QEMU |
| Compare/select less-than | `vector_select_lt` -> `vmslt.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select less-or-equal | `vector_select_le` -> `vmsle.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select greater-than | `vector_select_gt` -> swapped `vmslt.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select greater-or-equal | `vector_select_ge` -> swapped `vmsle.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select equality | `vector_select_eq` -> `vmseq.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select not-equal | `vector_select_ne` -> `vmsne.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select unsigned less-than | `vector_select_ult` -> `vmsltu.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select unsigned less-or-equal | `vector_select_ule` -> `vmsleu.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select unsigned greater-than | `vector_select_ugt` -> swapped `vmsltu.vv` + `vmerge.vvm` | objdump and QEMU |
| Compare/select unsigned greater-or-equal | `vector_select_uge` -> swapped `vmsleu.vv` + `vmerge.vvm` | objdump and QEMU |
| Masked arithmetic predicates | `vector_mask_*` + `vector_masked_add/sub/mul` slices -> RVV compare mask, masked arithmetic, `vmerge.vvm` passthrough | objdump and QEMU |
| Masked store | `vector_mask_*` + `vector_masked_store` -> RVV compare mask and predicated `vse32.v ..., v0.t` | objdump and QEMU |
| Masked load | `vector_mask_*` + `vector_masked_load` -> masked `vle32.v ..., v0.t` plus `vmerge.vvm` passthrough | objdump and QEMU |
| Masked non-unit memory | masked strided/indexed `i32` load/store forms -> `vlse32.v`, `vluxei32.v`, `vsse32.v`, `vsuxei32.v` with `v0.t` | objdump and QEMU |

## Current Gaps

| Area | Gap | Planned phase |
| --- | --- | --- |
| Element widths | typed-buffer contract exists and `i8/i64` add/copy/mul/reduce/scale/select slices are validated; masks and non-unit memory are not yet complete across every SEW | Phase 40C |
| LMUL policy | `m2`/`m4` are validated for i16 add only; other kernels remain mostly `m1` | Phase 42A |
| Memory forms | strided/indexed unmasked and masked loads/stores exist for `i32`; segment, FOF, and whole-register forms remain missing | Phase 47 |
| Masked arithmetic | add/sub/mul slices exist; no min/max/logic/widening/saturating/floating-point masked arithmetic yet | Phase 31C |
| Compare/select | signed and unsigned i32 select predicates are supported; reusable first-class masks exist as transient function-local compare symbols only | Phase 31D |
| Reductions | only add reduction is implemented | Phase 31A |
| ABI contract | direct-backend clobber policy is documented; full vector calling convention and spill/reload remain missing | Phase 49 |
| Formal lowering | MLIR/LLVM RVV path is still blocked by local toolchain mismatch | Phase 32A |
| Overflow policy | current RVV kernel subset checks wrapping modulo `2^32` | QEMU bit-pattern checks |
| Scalar `i32` arithmetic | direct RISC-V path uses RV64 word ops | QEMU stdout check |
| Compliance suite | no Spike or riscv-arch-test integration yet | Phase 32B |

## Required Test Matrix

Every RVV kernel should eventually be tested across:

- `n = 0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17`
- at least one length smaller than the active vector length
- at least one exact vector-length multiple
- at least one non-multiple tail
- at least one multi-iteration case
- positive and negative integer inputs when the operation semantics allow it
- QEMU execution result checks
- disassembly checks for expected RVV instruction families

Phase 29B records the source element-width contract: typed buffers first; the
validated execution subset is currently `i32` plus targeted `i16` add/widen
slices.

The current QEMU test covers the length set above for:

- `vector_add`
- `vector_copy`
- `vector_scale`
- `vector_mul`
- `vector_reduce_add`
- `vector_select_lt`
- `vector_select_le`
- `vector_select_gt`
- `vector_select_ge`
- `vector_select_eq`
- `vector_select_ne`
- `vector_select_ult`
- `vector_select_ule`
- `vector_select_ugt`
- `vector_select_uge`
- `vector_masked_add_lt`
- `vector_masked_add_le`
- `vector_masked_add_gt`
- `vector_masked_add_ge`
- `vector_masked_add_eq`
- `vector_masked_add_ne`
- `vector_masked_add_ult`
- `vector_masked_add_ule`
- `vector_masked_add_ugt`
- `vector_masked_add_uge`
- `vector_masked_sub_gt`
- `vector_masked_mul_gt`
- `vector_masked_store_gt`
- `vector_masked_load_gt`
- `matrix_multiply_packed_b`
- `matrix_pack_b_then_multiply`
- `vector_add_i16`
- `vector_add_i16_m2`
- `vector_add_i16_m4`
- `vector_add_i8`
- `vector_add_i64`
- `vector_copy_i8`
- `vector_copy_i64`
- `vector_mul_i8`
- `vector_mul_i64`
- `vector_scale_i8`
- `vector_scale_i64`
- `vector_reduce_add_i8`
- `vector_reduce_add_i64`
- `vector_select_i8_gt`
- `vector_select_i64_gt`
- `vector_strided_load`
- `vector_indexed_load`
- `vector_strided_store`
- `vector_indexed_store`
- `vector_masked_strided_load`
- `vector_masked_indexed_load`
- `vector_masked_strided_store`
- `vector_masked_indexed_store`
- `vector_mask_logical`
- `vector_widen_add_i16_i32`

## Acceptance Rule

A feature can be marked RVV 1.0 compatible only when all of the following are
true:

- The generated instruction sequence is legal for RVV 1.0.
- The source-level semantics are documented.
- The active accelerator profile records the supported SEW/LMUL/memory form.
- The test suite checks emitted assembly or objdump.
- The QEMU execution test checks runtime correctness.
- Any unsupported edge case is documented in this compliance matrix.

## Manual Validation

```bash
cd /home/zyz/zcompiler
python3 -m json.tool profiles/rvv-default.json >/dev/null
ctest --test-dir build -R qemu-riscv64 --output-on-failure
ctest --test-dir build --output-on-failure
```

## Phase 30R Compliance Note

`vector_masked_store_gt` is marked as compatible with the current RVV 1.0 subset because the emitted instruction sequence uses legal RVV 1.0 predicated unit-stride store syntax. The implementation remains limited to `i32`, LMUL `m1`, unit-stride memory, and transient function-local compare masks.

## Phase 30S Compliance Note

`vector_masked_load_gt` is compatible with the current RVV 1.0 subset as a unit-stride `i32` masked load slice. Because the active vtype policy is `ta, ma`, the direct RVV sequence explicitly merges the masked load result with the passthrough vector instead of relying on masked-off destination lanes.


## Phase 31T MMA Note

`matrix_multiply` is intentionally not counted as a new RVV 1.0 compliance row
yet. Phase 31T adds correct scalar row-major `i32` matrix multiply lowering and
QEMU validation. It becomes part of the RVV compliance matrix only after a future
phase lowers it to RVV instructions with documented memory/tile policy, objdump
checks, and QEMU correctness coverage.

## Phase 31U Packed-B MMA Compliance Note

`matrix_multiply_packed_b` is compatible with the current RVV 1.0 subset for
`i32`, LMUL `m1`, and unit-stride packed-B memory. The direct backend emits legal
RVV 1.0 `vsetvli`, `vle32.v`, `vmul.vv`, `vredsum.vs`, and `vmv.x.s` sequences,
validated by objdump checks and QEMU execution. It is not full matrix-kernel
coverage yet because the compiler still expects the caller or a future pack phase
to provide `packed_b`.


## Phase 37A Compliance Update

The current RVV 1.0-compatible subset now also includes:

- `vector_add` for validated `ptr<i16>` and `ptr<i32>` slices.
- `vector_add_m2` and `vector_add_m4` for validated `i16` LMUL addition.
- `vector_strided_load` for i32 element-strided loads using `vlse32.v`.
- `vector_indexed_load` for i32 indexed loads using `vluxei32.v`.
- Logical mask composition with `vmand.mm`, `vmor.mm`, `vmxor.mm`, and
  `vmnand.mm`.
- Signed `vector_widen_add_i16_i32` using `vwadd.vv`.
- Compiler-owned `matrix_pack_b` feeding the packed-B matrix multiply path.

The generated coverage table is maintained in
[rvv-1.0-compliance-generated.md](rvv-1.0-compliance-generated.md). Full RVV 1.0
compatibility is still not achieved; the largest remaining areas are complete
SEW/LMUL coverage, stores/gathers/scatters, floating point, fixed point,
permutation, exception/ABI policy, and the formal MLIR/LLVM RVV backend path.

## Phase 38A Compliance Update

`vector_add_m4` is now validated as a supported `ptr<i16>` RVV subset slice. The
emitted register groups begin at `v0`, `v4`, and `v8`, satisfying LMUL=4 group
alignment, and QEMU checks cover the same length set as the existing i16 `m1` and
`m2` kernels.

The broader `m2_m4_policy` row remains partial because LMUL expansion has not yet
been applied to every integer, memory, mask, reduction, and matrix operation.

## Phase 39A/39B Compliance Update

`vector_strided_store` and `vector_indexed_store` are now supported for the
current `ptr<i32>` RVV subset. `vector_strided_store` uses legal RVV 1.0
`vsse32.v`; `vector_indexed_store` uses legal RVV 1.0 `vsuxei32.v` with element
indices shifted to byte offsets. QEMU checks cover lengths
`0, 1, 2, 3, 5, 8, 17, 31` and verify unselected destination elements remain
unchanged.

## Phase 39C Compliance Update

`vector_masked_strided_load`, `vector_masked_indexed_load`,
`vector_masked_strided_store`, and `vector_masked_indexed_store` are now
supported for the current `ptr<i32>` RVV subset. Loads use `vlse32.v` or
`vluxei32.v` with `v0.t` plus explicit `vmerge.vvm` passthrough. Stores use
`vsse32.v` or `vsuxei32.v` with `v0.t` so false lanes preserve existing memory.
QEMU checks cover lengths `0, 1, 2, 3, 5, 8, 17, 31` and verify false-lane plus
tail-lane preservation.

## Phase 40A Compliance Update

The validated SEW matrix now includes `i8` and `i64` unit-stride slices.
`vector_add_i8`, `vector_add_i64`, `vector_copy_i8`, `vector_copy_i64`,
`vector_select_i8_gt`, and `vector_select_i64_gt` emit legal RVV 1.0
`vsetvli` configurations with `e8` or `e64` plus matching unit-stride load/store
instructions. QEMU checks cover lengths `0, 1, 2, 3, 5, 8, 17, 31` and verify
tail-lane preservation.

## Phase 40B1 Compliance Update

`vector_mul_i8`, `vector_mul_i64`, `vector_scale_i8`, and `vector_scale_i64` are
now validated unit-stride SEW slices. They emit legal RVV 1.0 `e8`/`e64`
configurations with matching `vle*.v`, `vmul.vv` or `vmul.vx`, and `vse*.v`.
QEMU checks cover lengths `0, 1, 2, 3, 5, 8, 17, 31` and verify tail-lane
preservation.

## Phase 40B2 Compliance Update

`vector_reduce_add_i8` and `vector_reduce_add_i64` are now validated
unit-stride SEW slices. The direct backend emits legal RVV 1.0 `e8`/`e64`
configurations with matching `vle*.v`, `vredsum.vs`, and scalar extraction.
This is a same-SEW reduction policy, not a widening reduction policy. QEMU checks
cover lengths `0, 1, 2, 3, 5, 8, 17, 31`.

## Phase 40C1 Compliance Update

The current RVV 1.0-compatible subset now includes typed unmasked strided memory
for `ptr<i8>`, `ptr<i16>`, `ptr<i32>`, and `ptr<i64>`. The direct backend emits
legal RVV 1.0 `vlse{SEW}.v` and `vsse{SEW}.v` instructions under `m1, ta, ma`,
with source strides counted in elements and converted to byte strides during
lowering. Runtime validation covers lengths `0, 1, 2, 3, 5, 8, 17, 31` under
QEMU.

Current memory gap after this phase: indexed and masked non-unit memory remain
validated only for the `i32` memory subset; segment, fault-only-first, and
whole-register memory forms are still planned work.
