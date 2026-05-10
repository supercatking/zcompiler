# Phase 20 RVV Lowering Design

Phase 20 starts the target-specific path from target-independent vector
operations to RISC-V RVV output.

## Phase 20A Scope

Implement a direct RVV reference assembly path for:

```zc
vector_add c, a, b, n;
```

The target instruction families are:

```asm
vsetvli
vle32.v
vadd.vv
vse32.v
```

## Why A Reference Backend First

The local MLIR build and the system RISC-V `llc` currently differ in LLVM
version and target support. A direct RVV reference backend gives the project a
stable executable target while the formal MLIR/LLVM RVV lowering path is built.

## Lowering Shape

```text
index = 0
while index < n:
  vl = vsetvli(n - index, e32, m1)
  va = vle32.v(a + index)
  vb = vle32.v(b + index)
  vc = vadd.vv(va, vb)
  vse32.v(vc, c + index)
  index += vl
```

## Follow-Up

- Phase 20B: add object/disassembly golden checks for RVV.
- Phase 20C: probe the formal MLIR/LLVM lowering path and document the current
  toolchain blockers in [phase20c-formal-rvv-lowering.md](phase20c-formal-rvv-lowering.md).
