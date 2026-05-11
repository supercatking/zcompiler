# Phase 30S: Masked Load

## Goal

Add a masked load memory consumer that complements `vector_masked_store`.

## Source Syntax

```zc
func masked_load_gt(mask_lhs: ptr<i32>, mask_rhs: ptr<i32>, input: ptr<i32>, passthrough: ptr<i32>, out: ptr<i32>, n: i32) -> i32 {
  vector_mask_gt m0, mask_lhs, mask_rhs, n;
  vector_masked_load out, input, m0, passthrough, n;
  return 0;
}
```

## Semantics

For each active element `i < n`:

- if `m0[i]` is true, `out[i] = input[i]`;
- if `m0[i]` is false, `out[i] = passthrough[i]`.

Elements outside `n` are tails and must remain unchanged.

## RVV Policy Detail

The current profile uses `ta, ma`. With mask-agnostic policy, masked-off destination lanes cannot be used as semantic passthrough. The direct RVV lowering therefore performs a masked load for true lanes and then uses `vmerge.vvm` to choose between loaded values and the passthrough vector.

## Validation

Validated by lexer/parser/codegen goldens, MLIR parsing, assembler/objdump checks, host correctness, and QEMU execution.
