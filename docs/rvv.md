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
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
```

Current direct RVV reference mappings:

- `vector_add`: `vle32.v`, `vadd.vv`, `vse32.v`
- `vector_copy`: `vle32.v`, `vse32.v`
- `vector_scale`: `vle32.v`, `vmul.vx`, `vse32.v`
- `vector_mul`: `vle32.v`, `vmul.vv`, `vse32.v`
- `vector_reduce_add`: `vle32.v`, `vmv.s.x`, `vredsum.vs`, `vmv.x.s`
- `vector_select_gt`: `vle32.v`, `vmslt.vv`, `vmerge.vvm`, `vse32.v`
- `vector_select_eq`: `vle32.v`, `vmseq.vv`, `vmerge.vvm`, `vse32.v`

All current vector kernels use a `vsetvli` loop and keep source-level syntax
independent from RVV instruction names.


## Compare/Select Kernel

Phase 30J implements the first predicate-oriented source operation:

```zc
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
```

It lowers to a signed vector compare plus select. The direct RVV reference path
uses `vmslt.vv v0, rhs, lhs` for greater-than, `vmseq.vv` for equality, and
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
