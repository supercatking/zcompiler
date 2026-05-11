# Phase 30O Masked Vector Add

## Goal

Add the first source-visible mask operation and one masked arithmetic consumer:

```zc
vector_mask_gt m0, mask_lhs, mask_rhs, n;
vector_masked_add out, a, b, m0, passthrough, n;
```

This is intentionally a narrow first slice. `m0` is a transient function-local
mask symbol, not a general value type yet. The design proves the frontend,
MLIR, direct RVV backend, host correctness, and QEMU execution shape before the
compiler grows a full mask type system.

## Semantics

For every active lane `i < n`:

```text
m0[i] = mask_lhs[i] > mask_rhs[i]
out[i] = m0[i] ? a[i] + b[i] : passthrough[i]
```

The arithmetic result uses the current `i32` wrapping bit-pattern policy already
used by the RVV QEMU tests. Lanes outside `n` are inactive.

## Lowering

The target-independent MLIR path uses:

- `vector.create_mask` for dynamic tails.
- `vector.transfer_read` and `vector.transfer_write` for unit-stride buffers.
- `arith.cmpi sgt` to create the lane mask.
- `arith.addi` for the active-lane addition.
- `arith.select` to preserve passthrough lanes.

The direct RVV reference backend lowers the fused pair with `v0` as the compare
mask:

```asm
vmslt.vv v0, v2, v1
vadd.vv v6, v3, v4, v0.t
vmerge.vvm v7, v5, v6, v0
vse32.v v7, (a0)
```

`vector_mask_gt` maps to swapped `vmslt.vv` because RVV provides less-than
compare instructions directly.

## Validation

The phase is validated through:

- lexer and parser golden tests for the new statements.
- MLIR and RISC-V assembly golden tests.
- `mlir-opt` parsing of the emitted masked-add MLIR.
- RVV assembler and objdump checks for `vmslt.vv`, masked `vadd.vv`,
  `vmerge.vvm`, and `vse32.v`.
- host correctness model in `test/correctness/vector_masked_add_gt_host.py`.
- QEMU execution in `test/qemu/run.sh` through the generated C harness.

Manual command:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_add_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Remaining Mask Work

This phase does not complete RVV mask coverage. Phase 30P implements the remaining compare mask predicates. Remaining work
includes reusable mask values across more operation kinds, logical mask ops,
masked loads/stores, mask policy tests, and eventually a typed MLIR mask
representation.
