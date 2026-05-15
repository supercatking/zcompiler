# Phase 39A/39B: Strided and Indexed Stores

## Goal

Fill the unmasked non-unit store gap in the current RVV memory subset.

## Source Surface

```zc
vector_strided_store base, values, stride, n;
vector_indexed_store base, values, indices, n;
```

`stride` and `indices` are expressed in elements. The direct RVV backend converts
them to byte offsets for RVV store instructions.

## Semantics

- `vector_strided_store base, values, stride, n` writes
  `base[i * stride] = values[i]` for `0 <= i < n`.
- `vector_indexed_store base, values, indices, n` writes
  `base[indices[i]] = values[i]` for `0 <= i < n`.
- Elements not selected by the first `n` logical lanes are not written.
- Phase 39A/39B intentionally validates only `ptr<i32>` data buffers.

## Direct RVV Lowering

- `vector_strided_store` emits `vle32.v` from the contiguous values buffer and
  `vsse32.v` into the strided destination.
- `vector_indexed_store` emits `vle32.v` for values and indices, shifts element
  indices into byte offsets with `vsll.vi`, then emits `vsuxei32.v`.

Both kernels use the existing dynamic `vsetvli` loop with `e32,m1,ta,ma`.

## Validation

Phase 39A/39B adds lexer/parser/codegen goldens, objdump-style instruction
checks, and QEMU execution. The QEMU harness runs lengths
`0, 1, 2, 3, 5, 8, 17, 31` and checks the full destination arrays so untouched
and tail positions remain unchanged.

Masked strided/indexed memory remains planned for Phase 39C.
