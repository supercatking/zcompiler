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
| SEW | `i32` source kernels | QEMU correctness tests |
| LMUL | `m1` | assembly inspection |
| Memory access | unit-stride `i32` buffers | QEMU correctness tests |
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

## Current Gaps

| Area | Gap | Planned phase |
| --- | --- | --- |
| Element widths | typed-buffer contract is defined; implementation remains `i32` only | Phase 29E |
| LMUL policy | only `m1` is emitted | Phase 29C |
| Memory forms | no strided or indexed vector load/store | Phase 30A |
| Masked arithmetic | add/sub/mul slices exist; no min/max/logic/widening/saturating/floating-point masked arithmetic yet | Phase 31C |
| Compare/select | signed and unsigned i32 select predicates are supported; reusable first-class masks exist as transient function-local compare symbols only | Phase 31D |
| Reductions | only add reduction is implemented | Phase 31A |
| ABI contract | vector register clobbering is not documented as an ABI | Phase 31B |
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

Phase 29B records the source element-width contract: typed buffers first; implemented width remains `i32`.

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
cd /home/zyz/zcomipler
python3 -m json.tool profiles/rvv-default.json >/dev/null
ctest --test-dir build -R qemu-riscv64 --output-on-failure
ctest --test-dir build --output-on-failure
```

## Phase 30R Compliance Note

`vector_masked_store_gt` is marked as compatible with the current RVV 1.0 subset because the emitted instruction sequence uses legal RVV 1.0 predicated unit-stride store syntax. The implementation remains limited to `i32`, LMUL `m1`, unit-stride memory, and transient function-local compare masks.
