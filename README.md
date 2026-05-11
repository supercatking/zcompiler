# zcompiler

`zcompiler` is a long-term compiler project with one final ambition:

> Build an AI self made compiler based on RISCV RVV accelerator.

The first practical milestone is a small MLIR-based toy compiler that can compile
a tiny language into LLVM IR and then into RISC-V assembly. This toy compiler is
not the final product. It is the bootstrapping path for learning, validating the
architecture, and gradually adding RISC-V Vector Extension (RVV) oriented
optimization passes.

## Long-Term Vision

The final system should become a compiler stack that combines:

- A small but extensible frontend language.
- MLIR dialects for high-level program representation.
- Lowering pipelines from custom dialects to standard MLIR dialects, LLVM
  dialect, LLVM IR, and RISC-V machine code.
- RISC-V RVV aware optimization passes.
- AI-assisted compiler construction, tuning, analysis, and optimization.
- Accelerator-oriented code generation for vector workloads.

## First Milestone

The first milestone is `toy-zc`, a minimal compiler inspired by MLIR's Toy
tutorial. It currently supports:

- Integer literals.
- Variables.
- Arithmetic expressions.
- Function definitions, parameters, and calls.
- Return statement.
- Straight-line assignment.
- `ptr<i32>` buffer parameters.
- Scalar indexed `load` / `store`.
- Target-independent `vector_add` syntax.
- Target-independent `vector_copy` syntax.
- Target-independent `vector_scale` syntax.
- Target-independent `vector_reduce_add` syntax.
- Built-in `print_i32` statement for RISC-V terminal output.
- Masked MLIR vector lowering for vector tails.
- MLIR emission.
- Lowering to LLVM-compatible IR.
- RISC-V assembly generation through LLVM's RISC-V backend.
- Direct RVV reference assembly for vector add.
- Direct RVV reference assembly for vector copy and vector scale.
- Direct RVV reference assembly for vector reduce add.
- Scalar-vs-vector benchmark metadata for vector add.
- Machine-readable RVV accelerator profile.
- Host-side correctness harnesses for masked vector tails.

Example target input:

```zc
func main() -> i32 {
  let x = 1 + 2 * 3;
  return x;
}
```

Expected first output path:

```text
source.zc
  -> AST
  -> zc MLIR dialect
  -> func / arith / scf MLIR dialects
  -> LLVM dialect
  -> LLVM IR
  -> RISC-V assembly
```

## Repository Documents

- [arch.md](arch.md): architecture design for the first toy compiler.
- [plan.md](plan.md): phased implementation plan and milestones.
- [docs/current-capabilities.md](docs/current-capabilities.md): current
  compiler capabilities, limits, and the most complex stable demo.

## Build Phase 1

This project uses the existing local LLVM/MLIR checkout and build:

```text
/home/zyz/mlir/llvm-project/
/home/zyz/mlir/build/
```

Configure and build:

```bash
cmake -G Ninja -S /home/zyz/zcomipler -B /home/zyz/zcomipler/build \
  -DMLIR_DIR=/home/zyz/mlir/build/lib/cmake/mlir \
  -DLLVM_DIR=/home/zyz/mlir/build/lib/cmake/llvm

cmake --build /home/zyz/zcomipler/build
```

Run the Phase 1 driver:

```bash
/home/zyz/zcomipler/build/tools/zc/zc --help
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
```

## Run Tests

```bash
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

Phase 2 adds the first lexer test:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-tokens
```

Phase 3 adds the first parser and AST dump:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-ast
```

Phases 4-7 add MLIR and LLVM IR emission:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-zc-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-lowered-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-riscv-asm
```

Later phases add control-flow examples:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/control.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/while.zc --emit-llvm
```

Current RVV vector-kernel path:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-riscv-asm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_copy.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_copy.zc --emit-riscv-asm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_scale.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_scale.zc --emit-riscv-asm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_reduce_add.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_reduce_add.zc --emit-riscv-asm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-riscv-asm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/print_i32.zc --emit-riscv-asm
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
/home/zyz/zcomipler/scripts/check-rvv-toolchain.sh
/home/zyz/zcomipler/scripts/prepare-riscv-llvm-build.sh --dry-run
/home/zyz/zcomipler/scripts/probe-formal-rvv-lowering.sh
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
```

Planning documents for the accelerator direction:

- [docs/rvv.md](docs/rvv.md)
- [docs/rvv-1.0-compliance.md](docs/rvv-1.0-compliance.md)
- [docs/current-capabilities.md](docs/current-capabilities.md)
- [docs/accelerator-profile.md](docs/accelerator-profile.md)
- [docs/correctness-testing.md](docs/correctness-testing.md)
- [docs/ai-workflow.md](docs/ai-workflow.md)
- [docs/phase18-vector-syntax.md](docs/phase18-vector-syntax.md)
- [docs/phase19-vector-mlir.md](docs/phase19-vector-mlir.md)
- [docs/phase20-rvv-lowering.md](docs/phase20-rvv-lowering.md)
- [docs/phase20c-formal-rvv-lowering.md](docs/phase20c-formal-rvv-lowering.md)
- [docs/phase25-vector-kernels.md](docs/phase25-vector-kernels.md)
- [docs/phase25c-vector-reduction.md](docs/phase25c-vector-reduction.md)
- [docs/phase26-rvv-toolchain.md](docs/phase26-rvv-toolchain.md)
- [docs/phase26b-riscv-llvm-build.md](docs/phase26b-riscv-llvm-build.md)
- [docs/phase28b-print-i32.md](docs/phase28b-print-i32.md)
- [docs/phase28c-qemu-rvv-execution.md](docs/phase28c-qemu-rvv-execution.md)
