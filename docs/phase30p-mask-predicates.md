# Phase 30P Mask Predicate Family

## Goal

Broaden the Phase 30O transient mask path from `vector_mask_gt` to the complete
current compare predicate family:

```zc
vector_mask_lt  m0, lhs, rhs, n;
vector_mask_le  m0, lhs, rhs, n;
vector_mask_gt  m0, lhs, rhs, n;
vector_mask_ge  m0, lhs, rhs, n;
vector_mask_eq  m0, lhs, rhs, n;
vector_mask_ne  m0, lhs, rhs, n;
vector_mask_ult m0, lhs, rhs, n;
vector_mask_ule m0, lhs, rhs, n;
vector_mask_ugt m0, lhs, rhs, n;
vector_mask_uge m0, lhs, rhs, n;
```

Each mask can feed the existing consumer:

```zc
vector_masked_add out, a, b, m0, passthrough, n;
```

## Architecture

No new AST ownership model is introduced. All new keywords lower into the same
`VectorMaskStmtAST` node with a `VectorSelectPredicate` value. `vector_masked_add`
continues to consume a function-local transient mask symbol. This keeps the mask
extension narrow and preserves the Phase 30N boundary: a full typed mask value
system remains future work.

## RVV Mapping

The direct RVV backend reuses the compare mapping already used by vector select:

| Source mask | RVV compare |
| --- | --- |
| `vector_mask_lt` | `vmslt.vv v0, v1, v2` |
| `vector_mask_le` | `vmsle.vv v0, v1, v2` |
| `vector_mask_gt` | `vmslt.vv v0, v2, v1` |
| `vector_mask_ge` | `vmsle.vv v0, v2, v1` |
| `vector_mask_eq` | `vmseq.vv v0, v1, v2` |
| `vector_mask_ne` | `vmsne.vv v0, v1, v2` |
| `vector_mask_ult` | `vmsltu.vv v0, v1, v2` |
| `vector_mask_ule` | `vmsleu.vv v0, v1, v2` |
| `vector_mask_ugt` | `vmsltu.vv v0, v2, v1` |
| `vector_mask_uge` | `vmsleu.vv v0, v2, v1` |

The consumer still emits masked `vadd.vv ..., v0.t` followed by `vmerge.vvm` to
preserve passthrough lanes.

## Validation

Phase 30P adds one source example for each predicate, lexer/parser/codegen
goldens, `mlir-opt` checks, assembler/objdump checks, host correctness, and QEMU
runtime checks for all masked-add predicate variants.
