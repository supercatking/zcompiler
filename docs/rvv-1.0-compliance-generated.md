# Generated RVV 1.0 Compliance Matrix

This file is generated from `profiles/rvv-compliance-matrix.json`.
Do not edit the table by hand; update the JSON and regenerate it.

| Area | Feature | Status | Phase | Validation |
| --- | --- | --- | --- | --- |
| `formal_lowering` | `mlir_vector_to_llvm_to_rvv` | `blocked` | `37A` | probe |
| `integer` | `add_sub_mul` | `supported` | `30Q` | objdump, qemu |
| `lmul` | `m1` | `supported` | `29A` | objdump, qemu |
| `lmul` | `m2_m4_policy` | `partial` | `33A/38A` | objdump, qemu_m2, qemu_m4, abi_doc |
| `lmul` | `m4_i16_vector_add` | `supported` | `38A` | lexer, parser, objdump, qemu |
| `mask` | `compare_masks` | `supported` | `30P` | objdump, qemu |
| `mask` | `mask_logical` | `supported` | `35A` | objdump, qemu |
| `matrix` | `compiler_owned_pack_b` | `supported` | `31V` | qemu |
| `matrix` | `packed_b_matmul` | `supported` | `31U` | objdump, qemu |
| `memory` | `indexed_load` | `supported` | `34B` | objdump, qemu |
| `memory` | `strided_load` | `supported` | `34A` | objdump, qemu |
| `memory` | `unit_stride_load_store` | `supported` | `20A` | objdump, qemu |
| `reduction` | `sum` | `supported` | `25C` | objdump, qemu |
| `sew` | `i32_e32` | `supported` | `29A` | objdump, qemu |
| `sew` | `i8_i16_i64` | `partial` | `32B/32C` | lexer, parser, qemu_i16 |
| `target` | `rv64gcv_lp64d` | `supported` | `29A` | assembler, qemu |
| `widening` | `widen_add` | `supported` | `36A` | objdump, qemu |

## Status Counts

- `supported`: 14
- `partial`: 2
- `planned`: 0
- `blocked`: 1
