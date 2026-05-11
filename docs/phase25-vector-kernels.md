# Phase 25 Vector Kernel Surface

Phase 25 starts expanding the vector kernel surface beyond a single
`vector_add` operation.

## Phase 25A Scope

Add:

```zc
vector_copy c, a, n;
```

This copies `n` `i32` elements from input buffer `a` to output buffer `c`.

## Architecture

`vector_copy` follows the same boundaries as `vector_add`:

- Lexer recognizes `vector_copy` as a keyword.
- Parser builds a target-independent AST statement.
- MLIRGen lowers to masked `vector.transfer_read` and
  `vector.transfer_write`.
- Direct RISC-V output emits RVV reference assembly using `vsetvli`,
  `vle32.v`, and `vse32.v`.

No RVV instruction name appears in source syntax or AST.

## MLIR Shape

```text
scf.for i = 0 to n step 4
  active = min(n - i, 4)
  mask = vector.create_mask active : vector<4xi1>
  value = vector.transfer_read a[i], mask : vector<4xi32>
  vector.transfer_write value, c[i], mask
```

## RVV Reference Shape

```asm
vsetvli vl, remaining, e32, m1, ta, ma
vle32.v v0, 0(input)
vse32.v v0, 0(output)
```

## Test Plan

- Lexer golden test for `vector_copy`.
- Parser/AST golden test for `VectorCopyStmt`.
- MLIR golden test with masked transfer lowering.
- RISC-V assembly golden test.
- Assembler and objdump checks for RVV load/store instructions.
- Host-side correctness harness for tail lengths.

## Phase 25B Scope

Add:

```zc
vector_scale c, a, factor, n;
```

This multiplies `n` `i32` elements from input buffer `a` by scalar `factor` and
writes the result to output buffer `c`.

## Phase 25B Architecture

`vector_scale` reuses the same frontend and lowering boundaries as the earlier
vector kernels:

- Lexer recognizes `vector_scale` as a keyword.
- Parser builds a `VectorScaleStmtAST` with output buffer, input buffer, scalar
  factor expression, and length expression.
- MLIRGen reuses the masked vector loop/read/write helpers.
- MLIRGen broadcasts the scalar factor with `vector.broadcast` and performs
  vector integer multiply with `arith.muli`.
- Direct RISC-V output emits RVV reference assembly using `vsetvli`,
  `vle32.v`, `vmul.vx`, and `vse32.v`.

The source syntax still does not expose RVV instruction names. `vmul.vx` is a
target-layer decision.

## Phase 25B MLIR Shape

```text
scf.for i = 0 to n step 4
  active = min(n - i, 4)
  mask = vector.create_mask active : vector<4xi1>
  value = vector.transfer_read a[i], mask : vector<4xi32>
  factor_vector = vector.broadcast factor : vector<4xi32>
  scaled = arith.muli value, factor_vector : vector<4xi32>
  vector.transfer_write scaled, c[i], mask
```

## Phase 25B RVV Reference Shape

```asm
vsetvli vl, remaining, e32, m1, ta, ma
vle32.v v0, 0(input)
vmul.vx v1, v0, factor
vse32.v v1, 0(output)
```

## Phase 25B Test Plan

- Lexer golden test for `vector_scale`.
- Parser/AST golden test for `VectorScaleStmt`.
- MLIR golden test with `vector.broadcast` and `arith.muli`.
- RISC-V assembly golden test with `vmul.vx`.
- Assembler and objdump checks for RVV load/multiply/store instructions.
- Host-side correctness harness for tail lengths and scalar factors.
