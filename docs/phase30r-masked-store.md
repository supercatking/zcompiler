# Phase 30R: Masked Store

## Goal

Add the first masked memory-side-effect consumer to the current RVV-compatible subset.

## Source Syntax

```zc
func masked_store_gt(mask_lhs: ptr<i32>, mask_rhs: ptr<i32>, values: ptr<i32>, out: ptr<i32>, n: i32) -> i32 {
  vector_mask_gt m0, mask_lhs, mask_rhs, n;
  vector_masked_store out, values, m0, n;
  return 0;
}
```

## Semantics

For each active element `i < n`:

- if `m0[i]` is true, write `values[i]` to `out[i]`;
- if `m0[i]` is false, leave `out[i]` unchanged.

Elements outside `n` are tails and must also remain unchanged.

## Architecture

`vector_masked_store` reuses transient function-local masks from Phase 30N-30Q. It adds `VectorMaskedStoreStmtAST` instead of overloading masked arithmetic because store is a side-effecting memory operation rather than a binary expression-like operation.

```text
Lexer keyword
  -> Parser statement
  -> VectorMaskedStoreStmtAST
  -> MLIRGen masked transfer_write
  -> direct RVV predicated store
```

## MLIR Lowering

MLIR lowering creates both masks:

- active tail mask from `vector.create_mask`;
- predicate mask from `arith.cmpi` over the mask producer inputs.

It combines them with `arith.andi` and uses the combined mask on `vector.transfer_write`.

## Direct RVV Lowering

Direct RVV assembly compares into `v0` and stores with:

```asm
vse32.v v3, 0(addr), v0.t
```

This is intentionally different from masked arithmetic, which computes a value and uses `vmerge.vvm` with a passthrough vector.

## Validation

Validated by:

- lexer/parser golden files;
- MLIR golden and `mlir-opt` parse check;
- RISC-V assembly golden;
- assembler and objdump checks for `vse32.v` plus `v0.t`;
- host correctness model preserving false lanes and tails;
- QEMU RVV execution harness.

Validated command:

```bash
cmake --build /home/zyz/zcompiler/build -j32
ctest --test-dir /home/zyz/zcompiler/build -j32 --output-on-failure
```

## Remaining Limits

The slice is still limited to `i32`, LMUL `m1`, unit-stride memory, and transient function-local compare masks. Full RVV 1.0 remains a roadmap target, not a completed feature.
