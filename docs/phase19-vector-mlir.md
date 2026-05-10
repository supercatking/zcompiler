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

## Phase 19B Tail Handling

Phase 19B removes the temporary `n % 4 == 0` constraint. The MLIR generator now
computes a per-iteration active lane count:

```text
remaining = n - i
active = min(remaining, 4)
mask = vector.create_mask active : vector<4xi1>
```

The mask is passed to both `vector.transfer_read` operations and the final
`vector.transfer_write`. This keeps the loop structure simple while making the
last iteration safe when the input length is not a multiple of four:

```mlir
%remaining = arith.subi %n, %i : index
%active = arith.minui %remaining, %c4 : index
%mask = vector.create_mask %active : vector<4xi1>
%lhs = vector.transfer_read %a[%i], %c0_i32, %mask
    : memref<?xi32>, vector<4xi32>
%rhs = vector.transfer_read %b[%i], %c0_i32, %mask
    : memref<?xi32>, vector<4xi32>
%sum = arith.addi %lhs, %rhs : vector<4xi32>
vector.transfer_write %sum, %c[%i], %mask
    : vector<4xi32>, memref<?xi32>
```

The fixed vector width remains an intermediate compiler policy, not a source
language feature. Later RVV lowering can map this masked vector IR either to
fixed-width vector code or to scalable RVV idioms such as `vsetvli`.

## Architecture Rule

The source language and AST still do not mention RVV. RVV-specific choices such
as `vsetvli`, LMUL, and vector length policy belong in Phase 20 target lowering.

## Test Plan

- Update `vector_add.zc --emit-mlir` golden output.
- Validate the generated MLIR with `mlir-opt`.
- Keep direct RVV assembly validation in Phase 20 until the formal MLIR/LLVM
  vector-to-RVV lowering path is selected.
