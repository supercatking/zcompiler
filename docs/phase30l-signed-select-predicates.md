# Phase 30L: Complete Signed Select Predicates

## Goal

Finish the signed compare/select predicate set for the current i32 RVV kernel
family before moving into first-class masks or unsigned comparisons.

## Source Operations

```zc
vector_select_lt out, lhs, rhs, true_values, false_values, n;
vector_select_le out, lhs, rhs, true_values, false_values, n;
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_ge out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
vector_select_ne out, lhs, rhs, true_values, false_values, n;
```

Each operation compares signed i32 lanes from `lhs` and `rhs`, then chooses the
corresponding lane from `true_values` or `false_values`.

## RVV Mapping

| Source op | MLIR predicate | RVV compare | Select |
| --- | --- | --- | --- |
| `vector_select_lt` | `arith.cmpi slt` | `vmslt.vv v0, lhs, rhs` | `vmerge.vvm` |
| `vector_select_le` | `arith.cmpi sle` | `vmsle.vv v0, lhs, rhs` | `vmerge.vvm` |
| `vector_select_gt` | `arith.cmpi sgt` | `vmslt.vv v0, rhs, lhs` | `vmerge.vvm` |
| `vector_select_ge` | `arith.cmpi sge` | `vmsle.vv v0, rhs, lhs` | `vmerge.vvm` |
| `vector_select_eq` | `arith.cmpi eq` | `vmseq.vv v0, lhs, rhs` | `vmerge.vvm` |
| `vector_select_ne` | `arith.cmpi ne` | `vmsne.vv v0, lhs, rhs` | `vmerge.vvm` |

`gt` and `ge` are expressed by swapping operands on RVV less-than or
less-or-equal compares, keeping the direct assembly path inside legal RVV 1.0
compare forms supported by the local assembler.

## Validation

Phase 30L adds examples, lexer/parser/codegen goldens, host correctness checks,
objdump checks, and QEMU runtime validation for the new signed predicates.

Validated command:

```bash
ctest --test-dir /home/zyz/zcompiler/build -j32 --output-on-failure
```

## Remaining Work

The compiler still needs unsigned compare/select predicates, first-class mask
values, explicit masked arithmetic, non-i32 element widths, richer memory forms,
and broader RVV 1.0 compliance testing.
