# Phase 30K: Vector Select Predicate Variants

## Goal

Broaden compare/select support beyond the first signed greater-than predicate
and make the compiler architecture ready for additional predicate variants.

## Implementation

Phase 30K refactors the select AST into a predicate-bearing node:

```text
VectorSelectStmtAST(predicate, output, lhs, rhs, true_values, false_values, n)
```

Supported source operations now include:

```zc
vector_select_gt out, lhs, rhs, true_values, false_values, n;
vector_select_eq out, lhs, rhs, true_values, false_values, n;
```

Direct RVV mappings:

| Source op | MLIR predicate | RVV compare | Select |
| --- | --- | --- | --- |
| `vector_select_gt` | `arith.cmpi sgt` | `vmslt.vv v0, rhs, lhs` | `vmerge.vvm` |
| `vector_select_eq` | `arith.cmpi eq` | `vmseq.vv v0, lhs, rhs` | `vmerge.vvm` |

## Validation

Phase 30K adds lexer/parser/codegen goldens, host correctness checks, objdump
checks, and QEMU runtime validation for `vector_select_eq`. The QEMU equality
case intentionally creates both true and false lanes.

Validated command:

```bash
ctest --test-dir /home/zyz/zcompiler/build -j32 --output-on-failure
```

## Remaining Predicate Work

The compiler still needs less-than, less-or-equal, greater-or-equal, not-equal,
unsigned variants, and eventually first-class mask values for explicit masked
arithmetic.
