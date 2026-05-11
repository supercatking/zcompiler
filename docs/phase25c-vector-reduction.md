# Phase 25C Vector Reduction

Phase 25C adds the first scalar-result vector kernel:

```zc
let sum = 0;
vector_reduce_add sum, a, n;
return sum;
```

## Goal

Add a target-independent source operation that reduces `n` `i32` elements from
input buffer `a` into scalar variable `sum`.

This extends the vector surface from memory-to-memory kernels to a
memory-to-scalar kernel.

## Source Contract

Syntax:

```zc
vector_reduce_add result, input, length;
```

Semantics:

- `result` is a scalar `i32` variable that already exists.
- `input` is a `ptr<i32>` buffer.
- `length` is an `i32` expression.
- The operation adds `input[0]` through `input[length - 1]` into `result`.
- If `length` is zero, `result` keeps its incoming value.

The first examples use:

```zc
let sum = 0;
vector_reduce_add sum, a, n;
return sum;
```

## AST Boundary

The frontend adds a `VectorReduceAddStmtAST` with:

- result variable name
- input buffer name
- length expression

The AST does not mention RVV, vector registers, `vredsum.vs`, or MLIR operation
classes.

## MLIR Shape

The MLIR lowering reuses the existing masked vector access helper and adds an
`scf.for` loop-carried scalar accumulator:

```text
acc0 = incoming result
accN = scf.for i = 0 to n step 4 iter_args(acc = acc0) -> i32
  active = min(n - i, 4)
  mask = vector.create_mask active : vector<4xi1>
  value = vector.transfer_read a[i], zero, mask : vector<4xi32>
  next = vector.reduction <add>, value, acc : vector<4xi32> into i32
  scf.yield next : i32
result = accN
```

Inactive lanes are padded with zero by the masked transfer read, so the
reduction remains tail-safe.

## Direct RVV Reference Shape

The direct RVV backend uses a scalar accumulator register and RVV reduction per
chunk:

```asm
vsetvli vl, remaining, e32, m1, ta, ma
vle32.v v0, 0(input)
vmv.s.x v1, acc
vredsum.vs v2, v0, v1
vmv.x.s acc, v2
```

This is still a reference backend. The formal MLIR-to-RVV path remains tracked
under Phase 26 because the local LLVM toolchain currently has an LLVM version
and RISC-V target mismatch.

## Validation Plan

- Lexer golden test for `vector_reduce_add`.
- Parser/AST golden test for `VectorReduceAddStmt`.
- MLIR golden test with `scf.for iter_args`, `vector.transfer_read`, and
  `vector.reduction <add>`.
- RISC-V assembly golden test with `vredsum.vs`.
- Assembler and objdump checks for RVV reduction instructions.
- Host-side correctness harness over tail lengths `0`, `1`, `3`, `4`, `5`,
  `7`, `16`, and `17`.
