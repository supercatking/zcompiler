# Phase 30Q Masked Arithmetic Consumers

## Goal

Broaden masked arithmetic from only `vector_masked_add` to:

```zc
vector_masked_add out, a, b, m0, passthrough, n;
vector_masked_sub out, a, b, m0, passthrough, n;
vector_masked_mul out, a, b, m0, passthrough, n;
```

## Architecture

Phase 30Q replaces the add-specific AST node with a generic
`VectorMaskedBinaryStmtAST` carrying a `VectorMaskedBinaryOp` enum. The parser,
MLIR generator, and direct RVV backend all dispatch on the operation enum. This
keeps the source syntax explicit while avoiding one AST/lowering class per
masked arithmetic operation.

The mask producer remains the transient function-local `vector_mask_*` symbol
from Phase 30P. A full typed mask value system is still future work.

## Lowering

The MLIR path maps the operation enum to:

- `arith.addi`
- `arith.subi`
- `arith.muli`

The direct RVV path maps the same enum to:

- `vadd.vv ..., v0.t`
- `vsub.vv ..., v0.t`
- `vmul.vv ..., v0.t`

Each consumer still finishes with `vmerge.vvm` to preserve passthrough lanes and
`vse32.v` to write the selected result.

## Validation

Phase 30Q validates:

- existing `vector_masked_add_*` examples after the generic AST refactor.
- `vector_masked_sub_gt.zc` and `vector_masked_mul_gt.zc` through lexer, parser,
  MLIR, direct RVV assembly, `mlir-opt`, assembler, objdump, host correctness,
  and QEMU execution.

Manual command:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_mul_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```
