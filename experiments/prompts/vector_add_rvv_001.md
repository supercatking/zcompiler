# Prompt Record: vector_add_rvv_001

## Prompt

```text
Continue the compiler roadmap autonomously, record plans and execution progress
in documentation, and keep working through later phases.
```

## Model

```text
Codex
```

## Input Files

```text
README.md
arch.md
plan.md
progress.md
docs/rvv.md
docs/ai-workflow.md
examples/
include/zcompiler/
lib/
test/
benchmarks/vector_add/
profiles/rvv-default.json
```

## Requested Change

```text
Advance the compiler toward the final AI-assisted RISC-V RVV goal, including
roadmap updates, code changes, tests, benchmark artifacts, and GitHub pushes.
```

## Generated Change Summary

```text
Implemented scalar pointer load/store, target-independent vector_add syntax,
MLIR vector lowering, direct RVV reference assembly, vector_add benchmark
artifacts, masked MLIR vector tails, scalar-vs-vector benchmark metadata, the
formal RVV lowering probe, and the first accepted experiment record.
```

## Validation

```bash
ctest --test-dir build --output-on-failure
./benchmarks/vector_add/run.sh
python3 -m json.tool build/benchmarks/vector_add/result.json
```

## Human Review Notes

```text
The RVV reference backend is accepted as a temporary target-specific path.
Formal MLIR/LLVM RVV lowering is now probed and blocked at a same-version
RISC-V `llc` toolchain requirement. Future vector/RVV comparisons should cite
`profiles/rvv-default.json`.
```

## Accepted

```text
yes
```
