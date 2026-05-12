# Phase 35A: Logical Mask Composition

## Goal

Allow compare masks to be composed before they feed masked arithmetic, stores,
or loads.

## Syntax

```zc
vector_mask_and m2, m0, m1, n;
vector_mask_or m3, m0, m1, n;
vector_mask_xor m4, m0, m1, n;
vector_mask_not m5, m0, n;
```

## Lowering

The direct RVV backend resolves mask symbols recursively and materializes the
final consumer mask in `v0`. Compare producers use `v8/v9` scratch data vectors;
logical masks use RVV 1.0 mask logical instructions such as `vmand.mm`,
`vmor.mm`, `vmxor.mm`, and `vmnand.mm` for not.

## Result

`examples/vector_mask_logical.zc` exercises all four logical operations and feeds
the composed mask into `vector_masked_add`. The QEMU harness checks the final
runtime result.
