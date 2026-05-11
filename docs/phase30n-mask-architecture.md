# Phase 30N: First-Class Mask Architecture

## Goal

Define how zcompiler should represent RVV masks before implementing explicit
masked arithmetic. This is an architecture phase because masks affect source
syntax, AST lifetime, MLIR lowering, direct RVV assembly, tests, and future
accelerator scheduling.

## Decision

Use transient, function-local mask symbols first. A mask symbol is produced by a
vector comparison statement and consumed by later masked vector statements in the
same function.

The first implementation should avoid memory-backed mask buffers. RVV mask
memory layout is bit-packed and brings extra ABI and layout rules that are not
needed for the first masked arithmetic slice.

## Source Contract

Planned syntax:

```zc
vector_mask_gt mask0, lhs, rhs, n;
vector_masked_add out, a, b, mask0, passthrough, n;
```

Rules:

- Mask names live only inside the current function body.
- A mask must be defined before use.
- The mask length is the same element count `n` used by the producer.
- The first implementation targets `i32` unit-stride buffers and `LMUL=m1`.
- `passthrough` provides inactive-lane values for the destination.
- Future phases may add unsigned mask producers and memory-backed mask spill/load.

## AST Shape

Planned nodes:

```text
VectorMaskStmtAST(predicate, mask_name, lhs, rhs, n)
VectorMaskedAddStmtAST(output, lhs, rhs, mask_name, passthrough, n)
```

Mask producers reuse the existing `VectorSelectPredicate` enum so compare
semantics stay centralized.

## MLIR Lowering

The initial MLIR path can lower a masked add by generating the compare mask and
masked arithmetic in the same vector loop when the producer and consumer share
compatible length and element type. Later phases can introduce a mask SSA cache
inside MLIRGen to support more scheduling freedom.

Expected operation family:

```text
arith.cmpi <predicate>
arith.addi
arith.select mask, sum, passthrough
vector.transfer_write
```

## Direct RVV Lowering

The direct RVV backend should keep masks in `v0` within the generated loop:

```text
vle32.v v1, (lhs_cmp)
vle32.v v2, (rhs_cmp)
<compare> v0, v1, v2
vle32.v v3, (a)
vle32.v v4, (b)
vadd.vv v5, v3, v4, v0.t
vle32.v v6, (passthrough)
vmerge.vvm v7, v6, v5, v0
vse32.v v7, (out)
```

The first implementation may fuse producer and consumer during lowering if the
mask is used exactly once by the immediately following masked operation. That is
an explicit implementation limit, not the long-term source contract.

## Validation Plan

- Lexer and parser goldens for `vector_mask_gt` and `vector_masked_add`.
- MLIR golden requiring `arith.cmpi`, `arith.addi`, `arith.select`, and masked vector transfers.
- RVV assembly golden requiring `vmslt.vv` or equivalent, `vadd.vv`, `vmerge.vvm`, and `v0` use.
- Host correctness with true and false lanes and tail lengths.
- QEMU runtime harness that checks inactive lanes come from `passthrough`.

## Open Design Points

- Whether reusable masks should become explicit SSA values in a future zc dialect.
- Whether memory-backed mask buffers should use RVV bit-packed mask layout or a
  source-level byte/i32 boolean layout with explicit conversion.
- How mask lifetime should interact with future loop fusion and accelerator scheduling.
