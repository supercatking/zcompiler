# Phase 30J: Vector Select Greater-Than

## Goal

Implement the Phase 30I compare/select design end to end as the first
predicate-oriented RVV kernel.

## Source Syntax

```zc
vector_select_gt out, lhs, rhs, true_values, false_values, n;
```

For each active lane `i` in `[0, n)`:

```text
out[i] = lhs[i] > rhs[i] ? true_values[i] : false_values[i]
```

The comparison is signed `i32`. The current implementation keeps the existing
RVV subset policy: unit-stride `i32` buffers, `SEW=32`, `LMUL=m1`, and dynamic
`vsetvli` chunking.

## Compiler Path

- Lexer recognizes `vector_select_gt` as a reserved keyword.
- Parser builds `VectorSelectGTStmtAST` with output, compare inputs, select
  inputs, and length expression.
- MLIR generation emits masked vector reads, `arith.cmpi sgt`, `arith.select`,
  and masked vector write.
- Direct RVV assembly emits `vmslt.vv v0, rhs, lhs`, then `vmerge.vvm` to select
  true or false vectors.

## Validation

Phase 30J adds:

- lexer/parser golden tests
- MLIR and RISC-V assembly golden tests
- objdump checks for `vmslt.vv` and `vmerge.vvm`
- host correctness checks for masked tail behavior
- QEMU runtime checks across the existing RVV length matrix

Validated command:

```bash
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```
