# Experiment: vector_kernel_surface_001

## Goal

Validate a profile-aware vector kernel surface beyond `vector_add`.

## Accelerator Profile

```text
profiles/rvv-default.json
```

Current profile operations:

```text
vector_add
vector_copy
vector_scale
vector_reduce_add
```

## Source Programs

```text
examples/vector_add.zc
examples/vector_copy.zc
examples/vector_scale.zc
examples/vector_reduce_add.zc
```

## Compiler Paths

```text
target-independent source syntax
  -> vector-specific AST statement
  -> masked MLIR vector dialect lowering
  -> direct RVV reference assembly
  -> assembler and objdump checks
  -> host-side correctness harness
```

## Expected RVV Instruction Families

```text
vector_add:        vsetvli, vle32.v, vadd.vv, vse32.v
vector_copy:       vsetvli, vle32.v, vse32.v
vector_scale:      vsetvli, vle32.v, vmul.vx, vse32.v
vector_reduce_add: vsetvli, vle32.v, vmv.s.x, vredsum.vs, vmv.x.s
```

## Relevant Commits

```text
733a8e9 Add vector add correctness harness
08a9f1b Add vector copy kernel support
1dce0f8 Add vector scale kernel support
3518c5f Add vector reduce add kernel support
c267d31 Add RVV toolchain diagnostic
```

## Validation Commands

```bash
cmake --build build -j2
ctest --test-dir build --output-on-failure
python3 -m json.tool build/correctness/vector_add_host.json
python3 -m json.tool build/correctness/vector_copy_host.json
python3 -m json.tool build/correctness/vector_scale_host.json
python3 -m json.tool build/correctness/vector_reduce_add_host.json
python3 -m json.tool profiles/rvv-default.json
```

## Result

Accepted.

The kernel surface now covers:

- memory-to-memory load/compute/store (`vector_add`, `vector_scale`)
- memory-to-memory copy (`vector_copy`)
- memory-to-scalar reduction (`vector_reduce_add`)

All accepted changes cite the default RVV accelerator profile and keep RVV
instruction names out of the source language and AST.

## Follow-Up

Phase26 must provide a same-version RISC-V-capable LLVM/MLIR backend before the
formal MLIR/LLVM RVV lowering path can replace the direct reference backend.

Related prompt record:

```text
experiments/prompts/vector_kernel_surface_001.md
```
