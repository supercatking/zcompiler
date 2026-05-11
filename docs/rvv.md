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

## Initial RVV Assembly Goals

Target RVV instruction families:

- `vsetvli`
- `vle32.v`
- `vse32.v`
- `vadd.vv`
- `vmul.vx`
- `vmul.vv`
- `vredsum.vs`

## Validation Plan

- Add scalar and vector reference tests.
- Use generated LLVM IR or assembly inspection for the first RVV checks.
- Add QEMU or Spike execution only after the scalar RISC-V path is stable.

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

It records the current `rv64gcv` development target, default `i32` vector-add
policy, MLIR tail-mask lowering shape, direct RVV reference backend choice, and
the formal MLIR/LLVM RVV lowering blocker.

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

## Current Implemented Kernel Surface

The implemented target-independent source operations are:

```zc
vector_add c, a, b, n;
vector_copy c, a, n;
vector_scale c, a, factor, n;
vector_reduce_add sum, a, n;
```

Current direct RVV reference mappings:

- `vector_add`: `vle32.v`, `vadd.vv`, `vse32.v`
- `vector_copy`: `vle32.v`, `vse32.v`
- `vector_scale`: `vle32.v`, `vmul.vx`, `vse32.v`
- `vector_reduce_add`: `vle32.v`, `vmv.s.x`, `vredsum.vs`, `vmv.x.s`

All current vector kernels use a `vsetvli` loop and keep source-level syntax
independent from RVV instruction names.
