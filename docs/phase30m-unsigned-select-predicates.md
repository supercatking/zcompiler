# Phase 30M: Unsigned Select Predicates

## Goal

Add unsigned i32 compare/select operations after the signed predicate family is
complete, while keeping the same generic `VectorSelectStmtAST` architecture.

## Source Operations

```zc
vector_select_ult out, lhs, rhs, true_values, false_values, n;
vector_select_ule out, lhs, rhs, true_values, false_values, n;
vector_select_ugt out, lhs, rhs, true_values, false_values, n;
vector_select_uge out, lhs, rhs, true_values, false_values, n;
```

The `u` prefix follows MLIR/LLVM predicate spelling and means that each `i32`
lane is compared as an unsigned 32-bit integer bit pattern.

## RVV Mapping

| Source op | MLIR predicate | RVV compare | Select |
| --- | --- | --- | --- |
| `vector_select_ult` | `arith.cmpi ult` | `vmsltu.vv v0, lhs, rhs` | `vmerge.vvm` |
| `vector_select_ule` | `arith.cmpi ule` | `vmsleu.vv v0, lhs, rhs` | `vmerge.vvm` |
| `vector_select_ugt` | `arith.cmpi ugt` | `vmsltu.vv v0, rhs, lhs` | `vmerge.vvm` |
| `vector_select_uge` | `arith.cmpi uge` | `vmsleu.vv v0, rhs, lhs` | `vmerge.vvm` |

`ugt` and `uge` use swapped operands with RVV unsigned less-than or
less-or-equal comparisons.

## Validation

Phase 30M adds source examples, lexer/parser/codegen goldens, host correctness
checks with unsigned 32-bit bit-pattern semantics, objdump checks, and QEMU
runtime validation.

Validated command:

```bash
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```

## Remaining Work

First-class mask values and explicit masked arithmetic are still missing. Full
RVV 1.0 coverage also still requires non-i32 SEW implementation, LMUL policy,
richer memory forms, more reductions, ABI documentation, and a formal compliance
suite.
