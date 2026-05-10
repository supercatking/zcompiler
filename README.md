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
tutorial. It should support:

- Integer literals.
- Variables.
- Arithmetic expressions.
- Function definition.
- Return statement.
- MLIR emission.
- Lowering to LLVM-compatible IR.
- RISC-V assembly generation through LLVM's RISC-V backend.

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

Planning documents for the accelerator direction:

- [docs/rvv.md](docs/rvv.md)
- [docs/ai-workflow.md](docs/ai-workflow.md)
