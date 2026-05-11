# Experiment: vector_add_rvv_001

## Goal

Create the first traceable path from source-level vector add syntax to RVV
assembly.

## Source Program

```text
examples/vector_add.zc
```

```zc
func vadd(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, n: i32) -> i32 {
  vector_add c, a, b, n;
  return 0;
}
```

## Compiler Path

```text
vector_add source syntax
  -> VectorAddStmtAST
  -> MLIR vector dialect lowering
  -> direct RVV reference assembly
  -> RISC-V object
  -> objdump inspection
```

## Accelerator Profile

```text
profiles/rvv-default.json
```

## Relevant Commits

```text
9175c88 Add target independent vector add syntax
51ad8a0 Lower vector add to MLIR vector dialect
208eb9e Add RVV reference assembly for vector add
f77808e Add vector add benchmark artifacts
80454f2 Handle vector add tails with MLIR masks
e860994 Add scalar vector add benchmark baseline
e673cf5 Probe formal MLIR RVV lowering path
```

## Validation Commands

```bash
ctest --test-dir build --output-on-failure
./benchmarks/vector_add/run.sh
python3 -m json.tool build/benchmarks/vector_add/result.json
```

## Expected RVV Instructions

```text
vsetvli
vle32.v
vadd.vv
vse32.v
```

## Result

Accepted as the first RVV reference path.

The path is intentionally split:

- MLIR vector dialect is available for target-independent inspection.
- Direct RVV assembly is available as a stable reference backend.
- Formal MLIR/LLVM RVV lowering is reproducibly probed and currently blocked
  at RISC-V `llc` toolchain mismatch.

## Notes

The local MLIR build and system RISC-V `llc` are not from the same LLVM target
configuration. The direct RVV reference backend avoids blocking progress while
the formal lowering pipeline is still being stabilized.

Related prompt record:

```text
experiments/prompts/vector_add_rvv_001.md
```
