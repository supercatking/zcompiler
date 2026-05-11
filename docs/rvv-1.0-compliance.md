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

## Current Gaps

| Area | Gap | Planned phase |
| --- | --- | --- |
| Element widths | source language only exposes `i32` vector kernels | Phase 29B |
| LMUL policy | only `m1` is emitted | Phase 29C |
| Memory forms | no strided or indexed vector load/store | Phase 30A |
| Masked arithmetic | no explicit masked source operations | Phase 30B |
| Compare/select | no vector compare/select kernels | Phase 30C |
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

The current QEMU test covers the length set above for:

- `vector_add`
- `vector_copy`
- `vector_scale`
- `vector_mul`
- `vector_reduce_add`

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
