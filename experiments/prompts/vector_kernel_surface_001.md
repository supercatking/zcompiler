# Prompt Record: vector_kernel_surface_001

## Prompt

```text
Continue the compiler roadmap autonomously, record plans and execution progress
in documentation, update RVV and AI workflow documents, add tests, and push
changes to GitHub.
```

## Model

```text
Codex
```

## Input Files

```text
README.md
arch.md
arch_cn.md
plan.md
progress.md
docs/rvv.md
docs/ai-workflow.md
docs/phase25-vector-kernels.md
profiles/rvv-default.json
examples/
include/zcompiler/
lib/
test/
```

## Requested Change

```text
Expand the vector kernel surface beyond vector_add while keeping source syntax
target-independent and RVV decisions in lowering/backend layers.
```

## Accelerator Profile

```text
profiles/rvv-default.json
```

## Generated Change Summary

```text
Added vector_copy, vector_scale, and vector_reduce_add kernels. Each kernel has
source examples, lexer/parser coverage, MLIR golden output, direct RVV reference
assembly coverage, assembler/objdump checks, and host-side correctness records.
The default RVV profile now lists all supported vector operations.
```

## Validation

```bash
cmake --build build -j2
ctest --test-dir build --output-on-failure
./benchmarks/vector_add/run.sh
./scripts/check-rvv-toolchain.sh
./scripts/probe-formal-rvv-lowering.sh
```

## Human Review Notes

```text
Accepted because each new source operation preserves frontend/backend
separation and has targeted regression coverage. Formal MLIR/LLVM RVV lowering
remains gated on Phase26 toolchain alignment, so direct RVV assembly stays the
reference backend for now.
```

## Accepted

```text
yes
```
