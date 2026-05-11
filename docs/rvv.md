# RVV Preparation

This document records the first RISC-V Vector Extension direction for
`zcompiler`.

## Source-Level Direction

Initial vector syntax proposal:

```zc
func main() -> i32 {
  let a = vector.load<i32, 8>(ptr_a);
  let b = vector.load<i32, 8>(ptr_b);
  let c = vector.add<i32, 8>(a, b);
  vector.store<i32, 8>(ptr_c, c);
  return 0;
}
```

The toy compiler does not implement this syntax yet. It defines the direction
for future phases.

## MLIR Lowering Direction

Planned lowering:

```text
zc vector source syntax
  -> zc.vector_* operations
  -> MLIR vector dialect
  -> LLVM dialect / RVV intrinsics
  -> RISC-V RVV assembly
```

## First Operation Set

- `zc.vector_load`
- `zc.vector_store`
- `zc.vector_copy`
- `zc.vector_add`
- `zc.vector_scale`
- `zc.vector_mul`
- `zc.vector_reduce_add`
- `zc.vector_select_gt`
- `zc.vector_mask_lt/le/gt/ge/eq/ne/ult/ule/ugt/uge`
- `zc.vector_masked_add`
- `zc.vector_masked_sub`
- `zc.vector_masked_mul`
- `zc.vector_masked_store`

## Initial RVV Assembly Goals

Target RVV instruction families:

- `vsetvli`
- `vle32.v`
- `vse32.v`
- `vadd.vv`
- `vmul.vx`
- `vmul.vv`
- `vredsum.vs`
- `vmslt.vv`
- `vmerge.vvm`
- masked arithmetic using `v0.t`
- predicated stores using `vse32.v ..., v0.t`

## Validation Plan

- Add scalar and vector reference tests.
- Use generated LLVM IR or assembly inspection for the first RVV checks.
- Keep QEMU execution validation in CTest for emitted RISC-V64/RVV binaries.
- Track RVV 1.0 coverage in
  [rvv-1.0-compliance.md](rvv-1.0-compliance.md).

## Phase Roadmap

RVV work is planned across several phases:

- Phase 18: add source-level vector syntax and vector AST nodes.
- Phase 19: lower vector operations to MLIR vector dialect.
- Phase 20: lower vector operations toward RVV intrinsics or RVV assembly.
- Phase 21: benchmark scalar and vector paths with a documented accelerator
  profile.

## Accelerator Profile

The current machine-readable profile is:

```text
profiles/rvv-default.json
```

It records the current `rv64gcv` development target, RVV 1.0 target spec,
default `i32` vector-add policy, MLIR tail-mask lowering shape, direct RVV
reference backend choice, QEMU validation matrix, and the formal MLIR/LLVM RVV
lowering blocker.

The profile stays separate from parser and AST code. Source programs express
vector intent; the target profile guides lowering, benchmark metadata, and
validation choices.

## Architecture Boundary

RVV-specific logic should live in target/lowering layers, not in lexer or parser
logic. The intended separation is:

```text
Source vector syntax
  -> target-independent AST
  -> target-independent zc/vector MLIR
  -> MLIR vector dialect
  -> RVV-specific lowering
```


## Source Element-Width Contract

Phase 29B chooses a typed-buffer-first contract for SEW expansion. Kernel names
remain width-neutral; the pointee type of `ptr<i8>`, `ptr<i16>`, `ptr<i32>`, or
`ptr<i64>` will drive MLIR element types and direct RVV `vsetvli` SEW selection.

The current implementation still supports only `ptr<i32>` vector kernels. Future
SEW phases should add one width at a time with profile, golden, objdump, host,
and QEMU checks.

## Current Implemented Kernel Surface

The implemented target-independent source operations are:

```zc
vector_add c, a, b, n;
vector_copy c, a, n;
vector_scale c, a, factor, n;
vector_mul c, a, b, n;
vector_reduce_add sum, a, n;
vector_select_lt out, lhs, rhs, true_values, false_values, n;
vector_select_le out, lhs, rhs, true_values, false_values, n;
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_ge out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
vector_select_ne out, lhs, rhs, true_values, false_values, n;
vector_select_ult out, lhs, rhs, true_values, false_values, n;
vector_select_ule out, lhs, rhs, true_values, false_values, n;
vector_select_ugt out, lhs, rhs, true_values, false_values, n;
vector_select_uge out, lhs, rhs, true_values, false_values, n;
vector_mask_lt m0, mask_lhs, mask_rhs, n;
vector_mask_le m0, mask_lhs, mask_rhs, n;
vector_mask_gt m0, mask_lhs, mask_rhs, n;
vector_mask_ge m0, mask_lhs, mask_rhs, n;
vector_mask_eq m0, mask_lhs, mask_rhs, n;
vector_mask_ne m0, mask_lhs, mask_rhs, n;
vector_mask_ult m0, mask_lhs, mask_rhs, n;
vector_mask_ule m0, mask_lhs, mask_rhs, n;
vector_mask_ugt m0, mask_lhs, mask_rhs, n;
vector_mask_uge m0, mask_lhs, mask_rhs, n;
vector_masked_add out, a, b, m0, passthrough, n;
vector_masked_sub out, a, b, m0, passthrough, n;
vector_masked_mul out, a, b, m0, passthrough, n;
vector_masked_store out, values, m0, n;
```

Current direct RVV reference mappings:

- `vector_add`: `vle32.v`, `vadd.vv`, `vse32.v`
- `vector_copy`: `vle32.v`, `vse32.v`
- `vector_scale`: `vle32.v`, `vmul.vx`, `vse32.v`
- `vector_mul`: `vle32.v`, `vmul.vv`, `vse32.v`
- `vector_reduce_add`: `vle32.v`, `vmv.s.x`, `vredsum.vs`, `vmv.x.s`
- `vector_select_lt`: `vle32.v`, `vmslt.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_le`: `vle32.v`, `vmsle.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_gt`: `vle32.v`, swapped `vmslt.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_ge`: `vle32.v`, swapped `vmsle.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_eq`: `vle32.v`, `vmseq.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_ne`: `vle32.v`, `vmsne.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_ult`: `vle32.v`, `vmsltu.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_ule`: `vle32.v`, `vmsleu.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_ugt`: `vle32.v`, swapped `vmsltu.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_uge`: `vle32.v`, swapped `vmsleu.vv`, `vmerge.vvm`, `vse32.v`
- `vector_mask_*` + `vector_masked_add/sub/mul`: compare into `v0`, masked `vadd.vv` / `vsub.vv` / `vmul.vv`, `vmerge.vvm`, `vse32.v`
- `vector_mask_*` + `vector_masked_store`: compare into `v0`, `vse32.v ..., v0.t` to preserve false lanes in memory

All current vector kernels use a `vsetvli` loop and keep source-level syntax
independent from RVV instruction names.


## Compare/Select Kernel

Phase 30M covers signed and unsigned i32 predicate-oriented source operations:

```zc
vector_select_lt out, lhs, rhs, true_values, false_values, n;
vector_select_le out, lhs, rhs, true_values, false_values, n;
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_ge out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
vector_select_ne out, lhs, rhs, true_values, false_values, n;
vector_select_ult out, lhs, rhs, true_values, false_values, n;
vector_select_ule out, lhs, rhs, true_values, false_values, n;
vector_select_ugt out, lhs, rhs, true_values, false_values, n;
vector_select_uge out, lhs, rhs, true_values, false_values, n;
vector_mask_lt m0, mask_lhs, mask_rhs, n;
vector_mask_le m0, mask_lhs, mask_rhs, n;
vector_mask_gt m0, mask_lhs, mask_rhs, n;
vector_mask_ge m0, mask_lhs, mask_rhs, n;
vector_mask_eq m0, mask_lhs, mask_rhs, n;
vector_mask_ne m0, mask_lhs, mask_rhs, n;
vector_mask_ult m0, mask_lhs, mask_rhs, n;
vector_mask_ule m0, mask_lhs, mask_rhs, n;
vector_mask_ugt m0, mask_lhs, mask_rhs, n;
vector_mask_uge m0, mask_lhs, mask_rhs, n;
vector_masked_add out, a, b, m0, passthrough, n;
vector_masked_sub out, a, b, m0, passthrough, n;
vector_masked_mul out, a, b, m0, passthrough, n;
```

It lowers to a signed or unsigned vector compare plus select. The direct RVV reference path
uses `vmslt.vv` / `vmsle.vv` for signed ordering, `vmsltu.vv` / `vmsleu.vv` for unsigned ordering, `vmseq.vv` / `vmsne.vv` for equality, and
`vmerge.vvm` to choose between the true and false vectors.

## RVV 1.0 Compliance Status

The current compiler should be treated as an RVV 1.0 compatible subset, not a
complete RVV 1.0 implementation. The tracked compliance matrix is:

```text
docs/rvv-1.0-compliance.md
```

Current committed subset:

- `SEW=32`
- `LMUL=m1`
- unit-stride memory
- `ta, ma` direct assembly policy
- QEMU-tested lengths: `0, 1, 2, 3, 4, 5, 7, 8, 9, 16, 17`


## Mask Architecture

First-class mask architecture is defined in
[phase30n-mask-architecture.md](phase30n-mask-architecture.md). Phase 30O implements
the first narrow slice with `vector_mask_gt` and `vector_masked_add`; see
[phase30o-masked-add.md](phase30o-masked-add.md). Phase 30P broadens the producer
side to all signed and unsigned compare predicates; see
[phase30p-mask-predicates.md](phase30p-mask-predicates.md). Masks are currently transient
function-local symbols and are lowered by the direct RVV backend through `v0` plus
masked arithmetic and `vmerge.vvm` passthrough selection.


## Masked Arithmetic Consumers

Phase 30Q adds a generic masked binary AST and implements `vector_masked_sub` and `vector_masked_mul` slices; see [phase30q-masked-arithmetic.md](phase30q-masked-arithmetic.md).

## Phase 30R Masked Store

Phase 30R adds the first masked memory-side-effect consumer:

```zc
vector_mask_gt m0, mask_lhs, mask_rhs, n;
vector_masked_store out, values, m0, n;
```

Semantics: active lanes where `m0` is true write `values[i]` into `out[i]`; false lanes keep the previous memory value. Tail lanes outside `n` are also preserved. MLIR lowers this through `arith.andi` combining the tail mask and compare mask before `vector.transfer_write`. Direct RVV uses a compare into `v0` plus predicated `vse32.v ..., v0.t`.
