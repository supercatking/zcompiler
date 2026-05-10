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
- `zc.vector_add`
- `zc.vector_mul`
- `zc.vector_reduce_add`

## Initial RVV Assembly Goals

Target RVV instruction families:

- `vsetvli`
- `vle32.v`
- `vse32.v`
- `vadd.vv`
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

## Accelerator Profile Draft

Future accelerator profile files should describe:

```yaml
name: rvv-generic
xlen: 64
vlen_bits: unknown
elen_bits: 32
supports:
  - i32_vector_add
  - i32_vector_mul
  - i32_vector_reduce_add
memory:
  alignment_bytes: 16
validation:
  assembler: riscv64-linux-gnu-as
  emulator: qemu-riscv64
```

The profile should stay separate from parser and AST code. Source programs
should express vector intent; the target profile should guide lowering and
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
