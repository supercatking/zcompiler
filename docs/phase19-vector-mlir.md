# Phase 19 Vector MLIR Lowering Design

Phase 19 lowers target-independent vector source operations to MLIR vector
dialect before any RVV-specific decision is made.

## Phase 19A Scope

Lower:

```zc
vector_add c, a, b, n;
```

to a fixed-width vector loop:

```text
scf.for i = 0 to n step 4
  va = vector.transfer_read a[i] : vector<4xi32>
  vb = vector.transfer_read b[i] : vector<4xi32>
  vc = arith.addi va, vb : vector<4xi32>
  vector.transfer_write vc, c[i]
```

## Temporary Constraint

Phase 19A assumes `n` is a multiple of 4. Tail handling is intentionally
deferred to Phase 19B, where the compiler should introduce masks or scalar
cleanup loops.

## Architecture Rule

The source language and AST still do not mention RVV. RVV-specific choices such
as `vsetvli`, LMUL, and vector length policy belong in Phase 20 target lowering.

## Test Plan

- Update `vector_add.zc --emit-mlir` golden output.
- Validate the generated MLIR with `mlir-opt`.
- Keep LLVM/RISC-V emission for vector operations deferred until Phase 20.
