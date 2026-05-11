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
