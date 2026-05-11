# Progress

This document records each development phase: execution target, execution
summary, and execution result.

## Phase 0: Repository Bootstrap

### Execution Target

Create the initial GitHub project foundation and document the long-term
compiler direction.

### Execution Summary

- Created `README.md`.
- Created `arch.md`.
- Created `plan.md`.
- Defined the final project goal: an AI self made compiler based on RISCV RVV
  accelerator.
- Pushed the initial documentation to GitHub.

### Execution Result

Completed.

Commit:

```text
fa08048 Add initial compiler architecture docs
```

## Phase 1: Build System and Tool Skeleton

### Execution Target

Create a minimal CMake-based compiler driver that links against the existing
local LLVM/MLIR build.

### Execution Summary

- Added top-level `CMakeLists.txt`.
- Added `tools/zc/zc.cpp`.
- Added `tools/zc/CMakeLists.txt`.
- Added `.gitignore`.
- Added `examples/hello.zc`.
- Connected the project to:
  - `/home/zyz/mlir/build/lib/cmake/llvm`
  - `/home/zyz/mlir/build/lib/cmake/mlir`
- Implemented initial CLI options:
  - `--emit=tokens`
  - `--emit=ast`
  - `--emit=mlir`
  - `--emit-tokens`
  - `--emit-ast`
  - `--emit-mlir`
  - `-o <file>`

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
/home/zyz/zcomipler/build/tools/zc/zc --help
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
```

Commit:

```text
865643f Add phase1 compiler driver skeleton
```

## Phase 2: Lexer

### Execution Target

Convert `.zc` source text into a token stream.

### Execution Summary

- Added `TokenKind` and `Token`.
- Added `Lexer`.
- Added keyword recognition for `func`, `let`, `return`, and `i32`.
- Added identifier and integer literal recognition.
- Added punctuation and operator recognition.
- Added `//` comment skipping.
- Added invalid character diagnostics with line and column.
- Implemented `zc --emit-tokens`.
- Added lexer CTest coverage.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-tokens
```

Commit:

```text
c0e275d Add phase2 lexer
```

## Phase 3: Parser and AST

### Execution Target

Parse tokens into a structured AST and expose the result through
`zc --emit-ast`.

### Execution Summary

- Added AST classes for:
  - module
  - function
  - let statement
  - return statement
  - integer expression
  - variable expression
  - binary expression
- Added a recursive descent parser.
- Added operator precedence parsing for `+`, `-`, `*`, and `/`.
- Implemented `zc --emit-ast`.
- Added parser CTest coverage for valid and invalid input.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-ast
```

Commit:

```text
9a9c88a Add phase3 parser and AST
```

## Phase 4: First MLIR Emission

### Execution Target

Generate the first standard MLIR output from the AST.

### Execution Summary

- Added `CodeGen` module.
- Added AST read-only accessors for code generation.
- Implemented standard MLIR text emission through `zc --emit-mlir`.
- Emitted builtin `module`, `func.func`, `arith.constant`, arithmetic ops, and
  `return`.
- Added codegen golden tests.
- Validated generated standard MLIR with `/home/zyz/mlir/build/bin/mlir-opt`.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/hello.mlir -o /tmp/hello.opt.mlir
```

## Phase 5: Custom zc Dialect

### Execution Target

Introduce the first project-owned `zc` dialect surface.

### Execution Summary

- Added initial ODS/TableGen design files:
  - `include/zcompiler/Dialect/ZC/IR/ZCDialect.td`
  - `include/zcompiler/Dialect/ZC/IR/ZCOps.td`
- Implemented `zc --emit-zc-mlir`.
- Added textual `zc` operations:
  - `zc.func`
  - `zc.constant`
  - `zc.add`
  - `zc.sub`
  - `zc.mul`
  - `zc.div`
  - `zc.return`
- Added golden tests for emitted `zc` MLIR text.

### Execution Result

Completed as the first vertical-slice dialect surface.

Note: this phase currently emits the project-owned dialect text and includes an
ODS scaffold. A later hardening pass should compile/register the dialect with
MLIR TableGen so `mlir-opt` can parse `zc` operations directly.

Validated command:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-zc-mlir
```

## Phase 6: Lowering Passes

### Execution Target

Lower the `zc` dialect surface to standard MLIR operations.

### Execution Summary

- Implemented `zc --emit-lowered-mlir`.
- Mapped `zc` arithmetic operations to `arith` operations.
- Mapped `zc.func` and `zc.return` to standard `func` dialect syntax.
- Added tests that verify lowered MLIR no longer contains `zc.` operations.
- Validated lowered MLIR with the same standard MLIR output path.

### Execution Result

Completed as AST-backed textual lowering.

Note: the next hardening step is to move this lowering into MLIR rewrite
patterns after the compiled `zc` dialect is registered.

Validated command:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-lowered-mlir
```

## Phase 7: LLVM IR Emission

### Execution Target

Emit LLVM IR for the first toy program.

### Execution Summary

- Implemented `zc --emit-llvm`.
- Generated a valid `define i32 @main()` function.
- Lowered integer arithmetic to LLVM IR instructions:
  - `add`
  - `sub`
  - `mul`
  - `sdiv`
- Added LLVM IR golden tests.
- Validated generated LLVM IR with `/home/zyz/mlir/build/bin/llvm-as`.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-llvm
/home/zyz/mlir/build/bin/llvm-as /tmp/hello.ll -o /tmp/hello.bc
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 21A: Reproducible Vector Add Artifacts

### Execution Target

Create the first reproducible benchmark artifact workflow for comparing RVV
reference C output with zcompiler RVV output.

### Execution Summary

- Added `benchmarks/vector_add/`.
- Added `reference_rvv.c`, a C implementation using RISC-V vector intrinsics.
- Added `benchmarks/vector_add/run.sh`.
- The script generates:
  - reference RVV assembly
  - reference RVV object
  - reference RVV objdump
  - zcompiler RVV assembly
  - zcompiler RVV object
  - zcompiler RVV objdump
- The script checks both paths for:
  - `vsetvli`
  - `vle32.v`
  - `vadd.vv`
  - `vse32.v`

### Execution Result

Completed as an artifact workflow. This is not a timing benchmark yet; it is the
first reproducible compile-and-inspect benchmark foundation.

Validated command:

```bash
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
```

## Phase 20B: RVV Instruction Regression Checks

### Execution Target

Make RVV instruction validation part of the normal codegen test suite.

### Execution Summary

- Extended `test/codegen/run.sh`.
- The test now checks generated `vector_add.riscv` for:
  - `vsetvli`
  - `vle32.v`
  - `vadd.vv`
  - `vse32.v`
- When `riscv64-linux-gnu-as` is available, the test assembles the RVV output
  with `-march=rv64gcv -mabi=lp64d`, runs `objdump`, and checks the same RVV
  instruction families in the disassembly.

### Execution Result

Completed.

Validated command:

```bash
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 20C: Formal MLIR/LLVM RVV Lowering Probe

### Execution Target

Investigate whether the masked MLIR vector IR can lower through MLIR and LLVM
to RISC-V RVV assembly without using the direct RVV reference backend.

### Execution Summary

- Added [docs/phase20c-formal-rvv-lowering.md](docs/phase20c-formal-rvv-lowering.md).
- Added `scripts/probe-formal-rvv-lowering.sh`.
- Verified that masked vector MLIR lowers to LLVM dialect.
- Verified that LLVM dialect translates to LLVM IR and bitcode.
- Confirmed that the LLVM IR contains `llvm.masked.load` and
  `llvm.masked.store`.
- Recorded the current backend blocker:
  - local LLVM 23 `llc` has no RISC-V target registered
  - system LLVM 14 `llc` has RISC-V but cannot consume LLVM 23 IR/bitcode

### Execution Result

Completed as a reproducible probe. The formal path is ready through LLVM IR and
blocked at RISC-V `llc` by toolchain mismatch.

Validated commands:

```bash
/home/zyz/zcomipler/scripts/probe-formal-rvv-lowering.sh
python3 -m json.tool /home/zyz/zcomipler/build/experiments/mlir-rvv/formal-rvv-lowering-result.json
```

## Phase 21B: Machine-Readable Benchmark Metadata

### Execution Target

Extend the vector-add benchmark workflow so generated artifacts can be consumed
by tools and future AI-assisted analysis.

### Execution Summary

- Added [docs/benchmark-workflow.md](docs/benchmark-workflow.md).
- Extended `benchmarks/vector_add/run.sh`.
- The benchmark now emits:
  - `build/benchmarks/vector_add/result.json`
  - `build/benchmarks/vector_add/result.md`
- `result.json` records:
  - benchmark id
  - source program
  - reference source
  - tools
  - output artifacts
  - object sizes
  - required RVV instruction counts
  - status
- Updated `benchmarks/vector_add/README.md`.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
python3 -m json.tool /home/zyz/zcomipler/build/benchmarks/vector_add/result.json
```

## Phase 21C: Scalar Baseline Comparison Metadata

### Execution Target

Add a scalar baseline to the vector-add benchmark so future accelerator work can
compare vector artifacts against a non-vector implementation.

### Execution Summary

- Added `benchmarks/vector_add/reference_scalar.c`.
- Extended `benchmarks/vector_add/run.sh` to emit scalar assembly, object, and
  objdump artifacts.
- Updated `result.json` to schema version 2 with:
  - scalar baseline artifacts
  - RVV reference artifacts
  - zcompiler RVV artifacts
  - scalar instruction counts
  - scalar RVV-instruction absence
  - zcompiler-vs-scalar object-size delta
  - zcompiler-vs-reference-RVV object-size delta
- Updated [docs/benchmark-workflow.md](docs/benchmark-workflow.md) and
  `benchmarks/vector_add/README.md`.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
python3 -m json.tool /home/zyz/zcomipler/build/benchmarks/vector_add/result.json
```

## Phase 22B: Prompt Records

### Execution Target

Add prompt records so AI-assisted compiler changes can be traced from request
to generated changes and validation.

### Execution Summary

- Added `experiments/prompts/README.md`.
- Added `experiments/prompts/TEMPLATE.md`.
- Added the first prompt record:
  - `experiments/prompts/vector_add_rvv_001.md`
- Updated [docs/ai-workflow.md](docs/ai-workflow.md).
- Linked the prompt record from:
  - `experiments/results/vector_add_rvv_001.md`

### Execution Result

Completed.

The first prompt record is:

```text
experiments/prompts/vector_add_rvv_001.md
```

Generated artifacts:

```text
/home/zyz/zcomipler/build/benchmarks/vector_add/reference_rvv.s
/home/zyz/zcomipler/build/benchmarks/vector_add/reference_rvv.o
/home/zyz/zcomipler/build/benchmarks/vector_add/reference_rvv.objdump
/home/zyz/zcomipler/build/benchmarks/vector_add/zcompiler_vector_add.s
/home/zyz/zcomipler/build/benchmarks/vector_add/zcompiler_vector_add.o
/home/zyz/zcomipler/build/benchmarks/vector_add/zcompiler_vector_add.objdump
```

## Phase 23A: Machine-Readable RVV Accelerator Profile

### Execution Target

Record the current RVV target assumptions in a machine-readable profile that
benchmarks, lowering probes, and AI experiment records can cite.

### Execution Summary

- Added `profiles/README.md`.
- Added `profiles/rvv-default.json`.
- Added [docs/accelerator-profile.md](docs/accelerator-profile.md).
- Updated `benchmarks/vector_add/run.sh` so generated `result.json` cites the
  accelerator profile.
- Updated architecture docs and diagrams to include accelerator profiles.
- Updated benchmark workflow docs to include `accelerator_profile` in generated
  metadata.

### Execution Result

Completed.

Validated commands:

```bash
python3 -m json.tool /home/zyz/zcomipler/profiles/rvv-default.json
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
python3 -m json.tool /home/zyz/zcomipler/build/benchmarks/vector_add/result.json
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 24A: Host-Side Vector Add Correctness Harness

### Execution Target

Add a correctness-oriented test that validates masked vector-add tail semantics,
even before a RISC-V emulator is available in the WSL environment.

### Execution Summary

- Added [docs/correctness-testing.md](docs/correctness-testing.md).
- Added `test/correctness/vector_add_host.py`.
- Added `test/correctness/run.sh`.
- Registered a new `correctness` CTest target.
- The host harness checks generated MLIR for:
  - `vector.create_mask`
  - `vector.transfer_read`
  - `vector.transfer_write`
  - `arith.minui`
- The host harness compares scalar semantics with masked `vector<4xi32>`
  chunk semantics for lengths `0`, `1`, `3`, `4`, `5`, `7`, `16`, and `17`.

### Execution Result

Completed as a host-side semantic correctness baseline. Emulator-backed RISC-V
execution remains a future extension because `qemu-riscv64` is not installed in
the current WSL environment.

Validated commands:

```bash
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
python3 -m json.tool /home/zyz/zcomipler/build/correctness/vector_add_host.json
```

## Phase 25A: Vector Copy Kernel Surface

### Execution Target

Expand the vector kernel surface beyond `vector_add` with a copy kernel that
reuses the same target-independent syntax, masked MLIR lowering, RVV reference
assembly, and correctness-test structure.

### Execution Summary

- Added `vector_copy c, a, n;` source syntax.
- Added lexer keyword support for `vector_copy`.
- Added `VectorCopyStmtAST`.
- Added parser support and AST dump output.
- Refactored `MLIRGen` masked vector loop/read/write helpers so `vector_add`
  and `vector_copy` share the same tail-safe lowering path.
- Added direct RVV reference assembly for `vector_copy` with:
  - `vsetvli`
  - `vle32.v`
  - `vse32.v`
- Added `examples/vector_copy.zc`.
- Added lexer, parser, MLIR, RISC-V assembly, assembler/objdump, and host-side
  correctness coverage for `vector_copy`.
- Updated `profiles/rvv-default.json` to list both `vector_add` and
  `vector_copy`.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j2
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/mlir/build/bin/mlir-opt /tmp/vector_copy.mlir -o /tmp/vector_copy.checked.mlir
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /tmp/vector_copy.s -o /tmp/vector_copy.o
python3 -m json.tool /home/zyz/zcomipler/build/correctness/vector_copy_host.json
```

## Phase 25B: Vector Scale Kernel Surface

### Execution Target

Add a compute kernel that multiplies an input vector by a scalar factor while
reusing the Phase 25 masked vector memory architecture.

### Execution Summary

- Added `vector_scale c, a, factor, n;` source syntax.
- Added lexer keyword support for `vector_scale`.
- Added `VectorScaleStmtAST` with output buffer, input buffer, scalar factor,
  and length expression.
- Added parser support and AST dump output for `VectorScaleStmt`.
- Reused `MLIRGen` masked vector loop/read/write helpers.
- Added MLIR lowering with:
  - `vector.create_mask`
  - `vector.transfer_read`
  - `vector.broadcast`
  - `arith.muli`
  - `vector.transfer_write`
- Added direct RVV reference assembly with:
  - `vsetvli`
  - `vle32.v`
  - `vmul.vx`
  - `vse32.v`
- Added `examples/vector_scale.zc`.
- Added lexer, parser, MLIR, RISC-V assembly, assembler/objdump, and host-side
  correctness coverage for `vector_scale`.
- Updated `profiles/rvv-default.json`, architecture docs, RVV docs,
  correctness docs, AI workflow docs, README, and roadmap.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j2
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_scale.zc --emit-mlir > /tmp/vector_scale.mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/vector_scale.mlir -o /tmp/vector_scale.checked.mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_scale.zc --emit-riscv-asm > /tmp/vector_scale.s
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /tmp/vector_scale.s -o /tmp/vector_scale.o
riscv64-linux-gnu-objdump -d /tmp/vector_scale.o
python3 -m json.tool /home/zyz/zcomipler/build/correctness/vector_scale_host.json
/home/zyz/zcomipler/benchmarks/vector_add/run.sh
python3 -m json.tool /home/zyz/zcomipler/build/benchmarks/vector_add/result.json
```

## Phase 25C: Vector Reduce Add Kernel Surface

### Execution Target

Add the first scalar-result vector kernel so the compiler can represent and
lower memory-to-scalar reductions.

### Execution Summary

- Added `vector_reduce_add sum, a, n;` source syntax.
- Added lexer keyword support for `vector_reduce_add`.
- Added `VectorReduceAddStmtAST`.
- Added parser support and AST dump output for `VectorReduceAddStmt`.
- Documented the reduction design in
  `docs/phase25c-vector-reduction.md` before implementation.
- Added MLIR lowering with:
  - `scf.for iter_args`
  - `vector.create_mask`
  - `vector.transfer_read`
  - `vector.reduction <add>`
  - `scf.yield`
- Added direct RVV reference assembly with:
  - `vsetvli`
  - `vle32.v`
  - `vmv.s.x`
  - `vredsum.vs`
  - `vmv.x.s`
- Added `examples/vector_reduce_add.zc`.
- Added lexer, parser, MLIR, RISC-V assembly, assembler/objdump, and host-side
  correctness coverage for `vector_reduce_add`.
- Updated `profiles/rvv-default.json`, architecture docs, RVV docs,
  correctness docs, AI workflow docs, README, and roadmap.

### Execution Result

Completed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j2
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_reduce_add.zc --emit-mlir > /tmp/vector_reduce_add.mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/vector_reduce_add.mlir -o /tmp/vector_reduce_add.checked.mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_reduce_add.zc --emit-riscv-asm > /tmp/vector_reduce_add.s
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /tmp/vector_reduce_add.s -o /tmp/vector_reduce_add.o
riscv64-linux-gnu-objdump -d /tmp/vector_reduce_add.o
python3 -m json.tool /home/zyz/zcomipler/build/correctness/vector_reduce_add_host.json
```

## Phase 26A: RVV Toolchain Diagnostic

### Execution Target

Make the formal MLIR-to-RVV lowering blocker reproducible and machine-readable.

### Execution Summary

- Added `scripts/check-rvv-toolchain.sh`.
- Added `docs/phase26-rvv-toolchain.md`.
- The diagnostic script records:
  - MLIR tool versions.
  - local `llc` version and RISC-V target status.
  - system `llc` version and RISC-V target status.
  - RISC-V assembler and objdump presence.
  - relevant local MLIR CMake cache values.
- Confirmed the current local MLIR build uses:
  - `LLVM_TARGETS_TO_BUILD=host`
  - `LLVM_DEFAULT_TARGET_TRIPLE=x86_64-unknown-linux-gnu`
- Confirmed current status:
  - local LLVM 23 `llc` has no RISC-V target.
  - system LLVM 14 `llc` has RISC-V but is version-incompatible with LLVM 23
    IR/bitcode.

### Execution Result

Completed as a diagnostic and planning phase. The formal lowering path remains
blocked until a same-version RISC-V-capable LLVM/MLIR toolchain is built or
installed.

Validated commands:

```bash
/home/zyz/zcomipler/scripts/check-rvv-toolchain.sh
python3 -m json.tool /home/zyz/zcomipler/build/experiments/rvv-toolchain/rvv-toolchain-diagnostic.json
/home/zyz/zcomipler/scripts/probe-formal-rvv-lowering.sh
```

## Phase 27A: Profile-Aware AI Experiment Records

### Execution Target

Require vector/RVV AI experiment records to cite the active accelerator profile
and demonstrate the standard with a concrete accepted record.

### Execution Summary

- Updated `experiments/prompts/TEMPLATE.md` with an `Accelerator Profile`
  section.
- Updated `experiments/README.md` to require accelerator profile references
  when target behavior matters.
- Updated `docs/ai-workflow.md` so experiment and prompt record formats include
  `accelerator_profile`.
- Added `experiments/prompts/vector_kernel_surface_001.md`.
- Added `experiments/results/vector_kernel_surface_001.md`.
- Recorded the accepted vector kernel surface:
  - `vector_add`
  - `vector_copy`
  - `vector_scale`
  - `vector_reduce_add`

### Execution Result

Completed as a workflow and traceability phase.

Validated commands:

```bash
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
python3 -m json.tool /home/zyz/zcomipler/profiles/rvv-default.json
```

## Phase 26B: RISCV LLVM Build Plan Script

### Execution Target

Make the same-version RISC-V-capable LLVM/MLIR build step reproducible while
keeping the existing `/home/zyz/mlir/build` directory untouched.

### Execution Summary

- Added `scripts/prepare-riscv-llvm-build.sh`.
- Added `docs/phase26b-riscv-llvm-build.md`.
- The script supports:
  - `--dry-run`
  - `--configure`
  - `--build`
- The default build plan uses:
  - source: `/home/zyz/mlir/llvm-project/llvm`
  - build: `/home/zyz/mlir/build-riscv`
  - projects: `mlir`
  - targets: `X86;RISCV`
  - required tools: `llc`, `mlir-opt`, `mlir-translate`, `llvm-as`
- The script writes reproducible plan artifacts under
  `build/experiments/rvv-toolchain/`.

### Execution Result

Completed as a dry-run build-plan phase. No external LLVM build was started by
default.

Validated commands:

```bash
/home/zyz/zcomipler/scripts/prepare-riscv-llvm-build.sh --dry-run
python3 -m json.tool /home/zyz/zcomipler/build/experiments/rvv-toolchain/riscv-llvm-build-plan.json
```

## Phase 22A: First AI-Assisted Experiment Record

### Execution Target

Create the first reproducible AI-assisted compiler experiment record so future
optimization work has a reviewable history.

### Execution Summary

- Created `experiments/`.
- Added `experiments/README.md`.
- Added `experiments/results/vector_add_rvv_001.md`.
- Updated [docs/ai-workflow.md](docs/ai-workflow.md) to point to the new
  experiment directory.
- Recorded:
  - source program
  - compiler path
  - relevant commits
  - validation commands
  - expected RVV instruction families
  - acceptance decision

### Execution Result

Completed.

The first accepted experiment is:

```text
experiments/results/vector_add_rvv_001.md
```

## Phase 20A: Direct RVV Reference Assembly

### Execution Target

Generate a first RVV-oriented assembly output for `vector_add` so the project
has a concrete accelerator-target reference.

### Execution Summary

- Added [docs/phase20-rvv-lowering.md](docs/phase20-rvv-lowering.md).
- Added direct RISC-V RVV reference assembly emission for `VectorAddStmtAST`.
- Generated a dynamic-vector-length loop using:
  - `vsetvli`
  - `vle32.v`
  - `vadd.vv`
  - `vse32.v`
- Added `vector_add.riscv` golden output.
- Added assembler validation with:
  - `riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d`

### Execution Result

Completed as a reference RVV backend.

The generated assembly follows this shape:

```asm
vsetvli t1, t1, e32, m1, ta, ma
vle32.v v0, 0(t3)
vle32.v v1, 0(t4)
vadd.vv v2, v0, v1
vse32.v v2, 0(t5)
```

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-riscv-asm
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /tmp/vector_add.rvv.s -o /tmp/vector_add.rvv.o
riscv64-linux-gnu-objdump -d /tmp/vector_add.rvv.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 19A: Vector Add to MLIR Vector Dialect

### Execution Target

Lower the target-independent `vector_add` source operation to MLIR vector
dialect without introducing RVV-specific source constructs.

### Execution Summary

- Added [docs/phase19-vector-mlir.md](docs/phase19-vector-mlir.md).
- Added MLIRGen support for `VectorAddStmtAST`.
- Lowered:

```zc
vector_add c, a, b, n;
```

to:

```text
scf.for i = 0 to n step 4
  vector.transfer_read a[i] : vector<4xi32>
  vector.transfer_read b[i] : vector<4xi32>
  arith.addi ... : vector<4xi32>
  vector.transfer_write ... c[i]
```

- Added `scf` and `vector` dialect dependencies to `MLIRGen`.
- Replaced the temporary vector codegen rejection test with a positive MLIR
  golden test.
- Validated the generated vector MLIR with `mlir-opt`.

### Execution Result

Completed for fixed-width `vector<4xi32>` chunks.

Temporary constraint: Phase 19A assumes the length is a multiple of 4. Tail and
mask handling are deferred to Phase 19B.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/vector_add.mlir -o /tmp/vector_add.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 19B: Masked Vector Tail Handling

### Execution Target

Make MLIR vector lowering safe when `vector_add` length is not a multiple of
the fixed intermediate vector width.

### Execution Summary

- Updated [docs/phase19-vector-mlir.md](docs/phase19-vector-mlir.md).
- Extended `MLIRGen` vector lowering to compute:
  - `remaining = n - i`
  - `active = min(remaining, 4)`
  - `mask = vector.create_mask active : vector<4xi1>`
- Passed the mask into both `vector.transfer_read` operations and the final
  `vector.transfer_write`.
- Updated the `vector_add` MLIR golden output to check the mask-producing IR.

### Execution Result

Completed for `vector<4xi32>` masked transfer lowering. This removes the Phase
19A `n % 4 == 0` assumption at the MLIR vector dialect level.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/vector_add.mlir -o /tmp/vector_add.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 18A: Target-Independent Vector Add Syntax

### Execution Target

Introduce the first source-level vector operation without exposing RVV
instruction names in the language.

### Execution Summary

- Added [docs/phase18-vector-syntax.md](docs/phase18-vector-syntax.md).
- Added the high-level statement:

```zc
vector_add c, a, b, n;
```

- Added lexer support for `vector_add`.
- Added `VectorAddStmtAST`.
- Added parser support for:
  - output buffer
  - left input buffer
  - right input buffer
  - length expression
- Added `examples/vector_add.zc`.
- Added lexer and parser golden tests.
- Added a codegen negative test that confirms vector lowering is intentionally
  rejected until Phase 19.

### Execution Result

Completed for source syntax and AST.

The compiler can now parse this target-independent vector operation:

```zc
func vadd(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, n: i32) -> i32 {
  vector_add c, a, b, n;
  return 0;
}
```

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-tokens
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/vector_add.zc --emit-ast
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 17B: Scalar Memory and Array Access

### Execution Target

Add the scalar memory baseline needed before vector syntax: typed buffer
parameters, scalar indexed loads, and scalar indexed stores.

### Execution Summary

- Extended the Phase 17 design with the `ptr<i32>` source type.
- Added lexer support for:
  - `ptr`
  - `load`
  - `store`
  - `[`
  - `]`
- Added AST nodes:
  - `LoadExprAST`
  - `StoreStmtAST`
- Added parser support for:
  - `ptr<i32>` parameter types
  - `load a[i]`
  - `store c[i] = value;`
- Added MLIRGen lowering:
  - `ptr<i32>` -> `memref<?xi32>`
  - `load` -> `memref.load`
  - `store` -> `memref.store`
  - `i32` index -> `index` through `arith.index_cast`
- Extended reference LLVM/RISC-V emitters for typed-pointer load/store.
- Hardened the RISC-V backend fallback: when MLIR v23 LLVM IR is incompatible
  with the system LLVM 14 `llc`, the backend retries with typed-pointer
  reference LLVM IR and still emits RISC-V assembly through `llc`.
- Added `examples/arrays.zc`.
- Added lexer, parser, MLIR, LLVM IR, and RISC-V assembly golden tests for
  `arrays.zc`.

### Execution Result

Completed.

This phase provides the scalar memory shape needed for future vector add:

```zc
func add_at(a: ptr<i32>, b: ptr<i32>, c: ptr<i32>, i: i32) -> i32 {
  let x = load a[i];
  let y = load b[i];
  let z = x + y;
  store c[i] = z;
  return z;
}
```

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/arrays.zc --emit-riscv-asm
/home/zyz/mlir/build/bin/mlir-opt /tmp/arrays.mlir -o /tmp/arrays.checked.mlir
/home/zyz/mlir/build/bin/llvm-as /tmp/arrays.ll -o /tmp/arrays.bc
riscv64-linux-gnu-as /tmp/arrays.s -o /tmp/arrays.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

Commit:

```text
fdf4299 Implement phases 4 through 7 codegen
d150b2e Track codegen golden outputs
```

## Phase 8: RISC-V Assembly

### Execution Target

Generate RISC-V assembly for the first toy program.

### Execution Summary

- Added `zc --emit-riscv-asm`.
- Added RISC-V text emission for integer constants, arithmetic, comparison,
  `if`, `while`, and `return`.
- Added `examples/control.zc` and `examples/while.zc`.
- Added golden tests for RISC-V assembly.
- Added optional validation with `riscv64-linux-gnu-as` when available.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-riscv-asm
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 9: Control Flow

### Execution Target

Add basic control flow and comparison operators.

### Execution Summary

- Added keywords:
  - `if`
  - `else`
  - `while`
- Added comparison operators:
  - `<`
  - `<=`
  - `>`
  - `>=`
  - `==`
  - `!=`
- Added AST nodes:
  - `IfStmtAST`
  - `WhileStmtAST`
- Added parser support for if/else blocks and while blocks.
- Added LLVM IR control-flow emission with labels and branches.
- Added parser and codegen tests for control-flow examples.

### Execution Result

Completed.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/control.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/control.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/while.zc --emit-llvm
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 10: Vector and RVV Preparation

### Execution Target

Define a concrete path from future source-level vector operations to RISC-V RVV
code generation.

### Execution Summary

- Added [docs/rvv.md](docs/rvv.md).
- Proposed future vector source syntax.
- Defined the planned lowering path:
  - zc vector source syntax
  - zc vector operations
  - MLIR vector dialect
  - LLVM dialect / RVV intrinsics
  - RISC-V RVV assembly
- Listed initial vector operation candidates.
- Listed first RVV instruction families to target.

### Execution Result

Completed as a design and implementation-preparation phase.

## Phase 11: AI-Assisted Compiler Direction

### Execution Target

Define the first AI-assisted compiler workflow for optimization experiments,
benchmark records, prompt records, and safety rules.

### Execution Summary

- Added [docs/ai-workflow.md](docs/ai-workflow.md).
- Defined experiment record format.
- Defined prompt record format.
- Added safety rules for AI-generated compiler changes.
- Listed near-term AI-assisted compiler uses.

### Execution Result

Completed as a workflow foundation.

Final validation:

```bash
cmake --build /home/zyz/zcomipler/build
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 12: Registered zc MLIR Dialect

### Execution Target

Turn the initial textual `zc` dialect surface into a registered MLIR dialect.

### Execution Summary

- Added TableGen-backed `ZCDialect`.
- Added registered `zc` operations:
  - `zc.constant`
  - `zc.add`
  - `zc.sub`
  - `zc.mul`
  - `zc.div`
- Added generated dialect/op headers and implementation files.
- Added `zc-opt`, a small MLIR optimizer driver that registers the `zc`,
  `arith`, and `func` dialects.
- Added dialect parser/printer test input.

### Execution Result

Completed for core arithmetic operations.

Note: this phase intentionally keeps functions in standard `func.func` while
`zc` owns computation operations. This keeps the first registered dialect small
and makes Phase 13 lowering clearer.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc-opt/zc-opt /home/zyz/zcomipler/test/dialect/registered.mlir -o /tmp/registered.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 13: Real MLIR Lowering Pass

### Execution Target

Lower registered `zc` arithmetic operations to standard MLIR `arith`
operations using an MLIR pass.

### Execution Summary

- Added `ZCToStandard` conversion library.
- Added `--lower-zc-to-standard` pass registration.
- Lowered:
  - `zc.constant` -> `arith.constant`
  - `zc.add` -> `arith.addi`
  - `zc.sub` -> `arith.subi`
  - `zc.mul` -> `arith.muli`
  - `zc.div` -> `arith.divsi`
- Extended dialect tests to assert lowered output contains no `zc.`
  operations.

### Execution Result

Completed for core arithmetic operations.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc-opt/zc-opt /home/zyz/zcomipler/test/dialect/registered.mlir --lower-zc-to-standard -o /tmp/lowered.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 14: AST to In-Memory MLIR

### Execution Target

Move the primary `--emit-mlir` path from hand-written MLIR text to MLIR C++ API
construction.

### Execution Summary

- Added `MLIRGen` module.
- Kept AST independent from MLIR headers.
- Built `mlir::ModuleOp` with `MLIRContext` and `OpBuilder`.
- Emitted `func.func`, `arith.constant`, arithmetic operations, comparisons,
  and `func.return` through MLIR operation builders.
- Updated `zc --emit-mlir` to print `ModuleOp`.
- Updated MLIR golden tests for MLIR printer-owned SSA names.

### Execution Result

Completed for straight-line arithmetic programs.

Note: `if` and `while` in-memory MLIR generation are intentionally deferred to
the vector/control-flow hardening path. Textual LLVM/RISC-V paths still support
the current control-flow examples.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/hello.mlir -o /tmp/hello.checked.mlir
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 15: MLIR to LLVM IR Pipeline

### Execution Target

Move the straight-line `--emit-llvm` path onto the official MLIR lowering and
translation tools instead of relying only on hand-written LLVM IR text.

### Execution Summary

- Added an MLIR-backed LLVM IR emission path in the `zc` driver.
- The driver now builds an in-memory MLIR module, writes it to a temporary MLIR
  file, invokes:
  - `/home/zyz/mlir/build/bin/mlir-opt --convert-to-llvm`
  - `/home/zyz/mlir/build/bin/mlir-translate --mlir-to-llvmir`
- Kept the existing text LLVM IR emitter as a fallback for language features
  that are not yet covered by in-memory MLIR generation, such as current
  `if`/`while` examples.
- Updated the `hello.zc` LLVM IR golden output to match MLIR's LLVM dialect
  translation result.

### Execution Result

Completed for straight-line arithmetic programs.

Note: full control-flow lowering through in-memory MLIR is intentionally kept as
a later hardening phase. The existing control-flow LLVM tests continue to pass
through the fallback path.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-llvm
/home/zyz/mlir/build/bin/llvm-as /tmp/new-hello.ll -o /tmp/new-hello.bc
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 16: LLVM RISC-V Backend Integration

### Execution Target

Generate RISC-V assembly through LLVM's RISC-V backend while keeping the
hand-written RISC-V emitter available as a reference fallback.

### Execution Summary

- Added a new `Target/RiscV` backend module.
- Moved the MLIR-backed LLVM IR emission helper behind a reusable target-layer
  API.
- Implemented the backend path:
  - AST
  - in-memory MLIR
  - `mlir-opt --convert-to-llvm`
  - `mlir-translate --mlir-to-llvmir`
  - `llc -mtriple=riscv64-unknown-elf -mattr=+m`
  - RISC-V assembly
- Updated `zc --emit-riscv-asm` to prefer the LLVM backend path and fall back
  to the hand-written reference emitter if the backend path is unavailable.
- Updated architecture documents and the architecture diagram to show the
  `Target/RiscV` module.
- Updated the `hello.zc` RISC-V assembly golden output to match LLVM backend
  output.

### Execution Result

Completed for straight-line arithmetic programs.

Environment note: `/home/zyz/mlir/build/bin/llc` currently does not include a
registered RISC-V target, so this phase uses `/usr/bin/llc`, which does include
`riscv64`. The MLIR conversion and LLVM IR translation still use the local MLIR
build under `/home/zyz/mlir/build/bin`.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/hello.zc --emit-riscv-asm
riscv64-linux-gnu-as /tmp/new-hello.riscv -o /tmp/new-hello.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 17: Functions, Calls, and Assignment

### Execution Target

Extend the language toward simple kernel structure with function parameters,
function calls, and straight-line assignment while documenting the next memory
model step.

### Execution Summary

- Added [docs/phase17-functions-memory.md](docs/phase17-functions-memory.md).
- Added lexer tokens for `:` and `,`.
- Added AST support for:
  - function parameters
  - call expressions
  - assignment statements
- Extended the parser to support:
  - `func add(a: i32, b: i32) -> i32`
  - `add(2, 3)` expression calls
  - `x = x + 4;` straight-line assignment
- Extended text emitters and MLIRGen for parameters, calls, and straight-line
  assignment.
- Added `examples/calls.zc`.
- Added lexer, parser, MLIR, LLVM IR, and RISC-V assembly golden coverage for
  `calls.zc`.

### Execution Result

Completed for the first Phase 17 slice.

Note: this phase intentionally models assignment as a new value bound to the
same source name in straight-line code. Explicit array/memref load-store syntax
is documented as the next memory-model slice.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-llvm
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/calls.zc --emit-riscv-asm
/home/zyz/mlir/build/bin/mlir-opt /tmp/calls.mlir -o /tmp/calls.checked.mlir
/home/zyz/mlir/build/bin/llvm-as /tmp/calls.ll -o /tmp/calls.bc
riscv64-linux-gnu-as /tmp/calls.s -o /tmp/calls.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 28A: Current Capability Demo

### Execution Target

Create a single high-coverage example that demonstrates the strongest currently
stable compiler path from toy source to MLIR, RVV assembly, and RISC-V object
generation.

### Execution Summary

- Added `examples/complex_vector_pipeline.zc`.
- Added [docs/current-capabilities.md](docs/current-capabilities.md).
- Updated `README.md` to link the current capability document and demo
  commands.
- Extended `test/codegen/run.sh` so the demo is covered by CTest.
- The demo covers:
  - multiple functions in one module
  - `ptr<i32>` parameters
  - `vector_add`
  - `vector_copy`
  - `vector_scale`
  - `vector_reduce_add`
  - masked MLIR vector lowering
  - direct RVV reference assembly
  - RISC-V object generation when the cross assembler is available

### Execution Result

Completed as the current "most complex stable demo" for the compiler.

Validated commands:

```bash
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-ast
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-mlir > /tmp/complex_vector_pipeline.mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/complex_vector_pipeline.mlir -o /tmp/complex_vector_pipeline.checked.mlir
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/complex_vector_pipeline.zc --emit-riscv-asm > /tmp/complex_vector_pipeline.s
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /tmp/complex_vector_pipeline.s -o /tmp/complex_vector_pipeline.o
riscv64-linux-gnu-objdump -d /tmp/complex_vector_pipeline.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 28B: Built-in print_i32 Runtime Output

### Execution Target

Let a `.zc` program print a computed nonzero integer result to the terminal
when compiled to RISC-V and run under QEMU.

### Execution Summary

- Added `print_i32 expr;` source syntax.
- Added lexer keyword support for `print_i32`.
- Added `PrintI32StmtAST` and AST dump support.
- Added parser support for print statements.
- Added `examples/print_i32.zc`.
- Added RISC-V text backend support:
  - register-preserving call site
  - emitted `zc_print_i32` runtime helper
  - signed decimal conversion
  - Linux `write` syscall
- Added lexer, parser, and codegen regression coverage.
- Documented the design in
  [docs/phase28b-print-i32.md](docs/phase28b-print-i32.md).

### Execution Result

Completed for RISC-V runtime output.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/print_i32.zc --emit-riscv-asm > /tmp/print_i32.s
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d /tmp/print_i32.s -o /tmp/print_i32_test
/home/qemu/qemu/build-riscv64-user/qemu-riscv64 -cpu max /tmp/print_i32_test
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 28C: QEMU RVV Execution Validation

### Execution Target

Add automated QEMU validation so zcompiler-generated RISC-V64 binaries are
executed, not only assembled.

### Execution Summary

- Added `test/qemu/run.sh`.
- Added a `qemu-riscv64` CTest target.
- The test uses:
  - `/home/qemu/qemu/build-riscv64-user/qemu-riscv64` by default
  - `ZCOMPILER_QEMU_RISCV64` override when needed
- The test validates `examples/print_i32.zc` by checking:
  - stdout is `220`
  - QEMU process exit status is `220`
- The test validates `examples/complex_vector_pipeline.zc` by linking the
  generated RVV assembly with a C correctness harness and checking QEMU exit
  status `0`.
- Added [docs/phase28c-qemu-rvv-execution.md](docs/phase28c-qemu-rvv-execution.md).
- Updated README and current capability docs.

### Execution Result

Completed for local QEMU-backed RVV execution validation.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```

## Phase 29A: RVV 1.0 Compliance Baseline

### Execution Target

Make the RVV 1.0 compatibility goal explicit and testable without claiming full
RVV 1.0 coverage before the compiler supports it.

### Execution Summary

- Updated `profiles/rvv-default.json` to schema version 2.
- Added RVV profile fields for:
  - `spec_version: 1.0`
  - `required_base_isa: RV64GCV`
  - supported LMUL for the current compiler subset
  - current compiler compliance status
  - QEMU execution validation matrix
- Added [docs/rvv-1.0-compliance.md](docs/rvv-1.0-compliance.md).
- Updated RVV, accelerator-profile, README, and plan documents.
- Extended `test/qemu/run.sh` so the complex RVV pipeline is executed across
  lengths `0`, `1`, `2`, `3`, `4`, `5`, `7`, `8`, `9`, `16`, and `17`.

### Execution Result

Completed as the baseline RVV 1.0 compliance tracking phase.

Validated commands:

```bash
python3 -m json.tool /home/zyz/zcomipler/profiles/rvv-default.json
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30A: Vector Multiply Kernel

### Execution Target

Add `vector_mul c, a, b, n;` as the next RVV 1.0 compatible subset kernel,
with source syntax, AST support, MLIR lowering, direct RVV assembly, host
correctness coverage, and QEMU execution validation.

### Execution Summary

- Added lexer keyword support for `vector_mul`.
- Added `VectorMulStmtAST` and AST dump support.
- Added parser support for `vector_mul c, a, b, n;`.
- Added MLIR lowering using masked transfer reads, `arith.muli`, and masked
  transfer write.
- Added direct RVV reference assembly using `vsetvli`, `vle32.v`, `vmul.vv`,
  and `vse32.v`.
- Added `examples/vector_mul.zc`.
- Added lexer, parser, MLIR, RISC-V assembly, host correctness, and QEMU
  regression coverage.
- Updated RVV profile, compliance docs, README, correctness docs, and
  [docs/phase30a-vector-mul.md](docs/phase30a-vector-mul.md).

### Execution Result

Completed for elementwise vector multiply over `i32` unit-stride buffers.

Validated commands:

```bash
python3 -m json.tool /home/zyz/zcomipler/profiles/rvv-default.json >/dev/null
cmake --build /home/zyz/zcomipler/build -j32
/home/zyz/mlir/build/bin/mlir-opt /home/zyz/zcomipler/test/codegen/vector_mul.mlir -o /tmp/vector_mul.checked.mlir
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d /home/zyz/zcomipler/test/codegen/vector_mul.riscv -o /tmp/vector_mul.o
riscv64-linux-gnu-objdump -d /tmp/vector_mul.o
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30B: QEMU Execution Manifest

### Execution Target

Move QEMU execution settings and RVV length coverage into a structured manifest
so runtime validation is easier to extend as new kernels are added.

### Execution Summary

- Added `test/qemu/rvv_execution_manifest.json`.
- Updated `test/qemu/run.sh` to read QEMU CPU mode, `print_i32` expectations,
  validated kernel metadata, and RVV length matrix from the manifest.
- Updated the temporary C harness generation so `lengths[]` comes from manifest
  data instead of a hard-coded C literal.
- Added [docs/phase30b-qemu-manifest.md](docs/phase30b-qemu-manifest.md).
- Updated QEMU validation docs, README, and plan.

### Execution Result

Completed as the first data-driven QEMU validation step. Per-kernel generated C
checks remain planned for Phase 30C.

Validated commands:

```bash
python3 -m json.tool /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json >/dev/null
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30C: QEMU Kernel Descriptors

### Execution Target

Add validated per-kernel descriptors to the QEMU execution manifest and move
manifest parsing out of `test/qemu/run.sh`.

### Execution Summary

- Added `test/qemu/manifest.py`.
- Extended `test/qemu/rvv_execution_manifest.json` with `kernel_checks`.
- Updated `test/qemu/run.sh` to call the helper instead of embedding Python.
- Added [docs/phase30c-qemu-kernel-descriptors.md](docs/phase30c-qemu-kernel-descriptors.md).
- Updated README and plan.

### Execution Result

Completed as validated metadata for QEMU runtime kernel coverage. Generated C
check fragments remain planned for Phase 30D.

Validated commands:

```bash
python3 /home/zyz/zcomipler/test/qemu/manifest.py /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30D: Signed i32 Runtime Coverage

### Execution Target

Extend QEMU runtime validation beyond positive-only inputs and document the
current `i32` integer semantics boundary.

### Execution Summary

- Updated `test/qemu/run.sh` so the C harness initializes mixed positive and
  negative input arrays.
- Updated runtime scale factors to include negative factors.
- Added `rvv_execution.integer_semantics` to the QEMU manifest.
- Extended `test/qemu/manifest.py` validation for the integer semantics policy.
- Added [docs/phase30d-signed-i32-semantics.md](docs/phase30d-signed-i32-semantics.md).
- Updated RVV compliance and plan documents.

### Execution Result

Completed for signed no-overflow runtime coverage. Source-level overflow
behavior remains a planned policy decision.

Validated commands:

```bash
python3 /home/zyz/zcomipler/test/qemu/manifest.py /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30E: i32 Wrapping Semantics

### Execution Target

Define and validate wrapping `i32` bit-pattern behavior for the current RVV
kernel subset.

### Execution Summary

- Reworked the QEMU C harness to compute expected arithmetic with `uint32_t`
  bit-pattern helpers.
- Seeded QEMU inputs near positive and negative `i32` boundaries.
- Added negative factors and overflow-producing add/multiply/reduction cases.
- Updated `rvv_execution.integer_semantics` in the QEMU manifest.
- Added [docs/phase30e-i32-wrapping-semantics.md](docs/phase30e-i32-wrapping-semantics.md).
- Updated RVV compliance and plan documents.

### Execution Result

Completed for the current RVV kernel subset. Scalar frontend-wide overflow
semantics remain a planned decision.

Validated commands:

```bash
python3 /home/zyz/zcomipler/test/qemu/manifest.py /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30F: Scalar i32 Wrapping Semantics

### Execution Target

Make scalar source `i32` arithmetic match the current RVV kernel wrapping
bit-pattern policy in direct RISC-V output.

### Execution Summary

- Changed direct RISC-V scalar `+`, `-`, `*`, and `/` lowering to `addw`,
  `subw`, `mulw`, and `divw`.
- Changed equality/inequality helper subtraction to `subw`.
- Added `examples/scalar_i32_wrap.zc`.
- Added RISC-V assembly golden coverage for scalar wrapping instructions.
- Added QEMU stdout validation for scalar `i32` wrapping results.
- Added [docs/phase30f-scalar-i32-wrapping.md](docs/phase30f-scalar-i32-wrapping.md).
- Updated RVV compliance, README, plan, and progress docs.

### Execution Result

Completed for the current direct RISC-V scalar runtime path.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
/home/zyz/zcomipler/build/tools/zc/zc /home/zyz/zcomipler/examples/scalar_i32_wrap.zc --emit-riscv-asm
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30G: QEMU Harness Generator

### Execution Target

Generate the QEMU RVV C harness from validated manifest data instead of keeping
the full C program embedded in `test/qemu/run.sh`.

### Execution Summary

- Added `test/qemu/harness.py`.
- Updated `test/qemu/run.sh` to call the generator.
- The generated harness uses manifest kernel-check comments, length matrix, and
  derived array capacity.
- Added [docs/phase30g-qemu-harness-generator.md](docs/phase30g-qemu-harness-generator.md).
- Updated README, plan, and progress docs.

### Execution Result

Completed for generated QEMU C harness orchestration.

Validated commands:

```bash
python3 /home/zyz/zcomipler/test/qemu/harness.py /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json /tmp/zcompiler_qemu_harness.c
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30H: QEMU Check Templates

### Execution Target

Refactor the generated QEMU C harness into explicit check-rendering units so new
kernel families can be added without editing one monolithic C template.

### Execution Summary

- Refactored `test/qemu/harness.py` into separate render helpers.
- Kept generated C behavior equivalent to Phase 30G.
- Added [docs/phase30h-qemu-check-templates.md](docs/phase30h-qemu-check-templates.md).
- Updated README, plan, and progress docs.

### Execution Result

Completed for QEMU harness generator structure.

Validated commands:

```bash
python3 /home/zyz/zcomipler/test/qemu/harness.py /home/zyz/zcomipler/test/qemu/rvv_execution_manifest.json /tmp/zcompiler_qemu_harness.c
ctest --test-dir /home/zyz/zcomipler/build -R qemu-riscv64 --output-on-failure
ctest --test-dir /home/zyz/zcomipler/build --output-on-failure
```


## Phase 30I: Vector Compare/Select Design

### Execution Target

Define the first RVV predicate kernel before changing compiler code.

### Execution Summary

- Chose `vector_select_gt out, lhs, rhs, true_values, false_values, n;` as the first compare/select operation.
- Documented signed `i32` semantics, AST ownership, MLIR vector lowering, direct RVV lowering, and QEMU acceptance checks.
- Updated README, RVV direction, compliance roadmap, and plan docs.

### Execution Result

Design completed. Implementation moves to Phase 30J.

Validated commands:

```bash
git diff --check
```


## Phase 30J: Vector Select Greater-Than

### Execution Target

Implement the first RVV predicate/select kernel end to end.

### Execution Summary

- Added `vector_select_gt out, lhs, rhs, true_values, false_values, n;`.
- Added lexer keyword, AST node, parser support, MLIR vector lowering, and direct RVV assembly.
- Added golden lexer/parser/codegen tests, host correctness checks, objdump checks, and QEMU runtime validation.
- Updated RVV profile, compliance matrix, correctness docs, README, and plan.

### Execution Result

Completed for signed `i32` greater-than vector select in the current `e32,m1` unit-stride subset.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```


## Phase 29B: Source Element-Width Contract

### Execution Target

Decide how source programs will expose RVV element widths beyond the current
`i32` implementation.

### Execution Summary

- Chose typed buffers as the source-level SEW contract.
- Kept vector operation names width-neutral.
- Added a profile contract for current and planned widths.
- Updated RVV docs, compliance roadmap, README, plan, and progress.

### Execution Result

Contract completed. Current compiler support remains `i32`; implementation of
non-`i32` SEW paths moves to a later phase.

Validated commands:

```bash
python3 -m json.tool /home/zyz/zcomipler/profiles/rvv-default.json >/dev/null
git diff --check
```


## Phase 30K: Vector Select Predicate Variants

### Execution Target

Broaden compare/select beyond signed greater-than and make the AST/lowering path
ready for additional predicates.

### Execution Summary

- Refactored vector select into `VectorSelectStmtAST` with a predicate enum.
- Kept `vector_select_gt` and added `vector_select_eq`.
- Added MLIR lowering for `arith.cmpi eq` and direct RVV `vmseq.vv`.
- Added lexer/parser/codegen goldens, host correctness, objdump checks, QEMU checks, profile updates, and docs.

### Execution Result

Completed for signed greater-than and equality vector select predicates.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```


## Phase 30L: Complete Signed Select Predicates

### Execution Target

Complete the signed i32 compare/select predicate family on top of the generic
vector select AST introduced in Phase 30K.

### Execution Summary

- Added `vector_select_lt`, `vector_select_le`, `vector_select_ge`, and `vector_select_ne`.
- Mapped signed predicates to MLIR `arith.cmpi` and RVV 1.0 compare instructions.
- Added examples, lexer/parser/codegen goldens, host correctness scripts, objdump checks, QEMU harness checks, profile updates, and documentation.

### Execution Result

Completed. Full build and CTest validation passed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```


## Phase 30M: Unsigned Select Predicates

### Execution Target

Add unsigned i32 compare/select operations using the generic vector select AST
and RVV unsigned compare mask instructions.

### Execution Summary

- Added `vector_select_ult`, `vector_select_ule`, `vector_select_ugt`, and `vector_select_uge`.
- Mapped MLIR unsigned predicates to `vmsltu.vv` and `vmsleu.vv`, swapping operands for greater-than forms.
- Added examples, lexer/parser/codegen goldens, host correctness scripts, objdump checks, QEMU harness checks, profile updates, and documentation.

### Execution Result

Completed. Full build and CTest validation passed.

Validated commands:

```bash
cmake --build /home/zyz/zcomipler/build -j32
ctest --test-dir /home/zyz/zcomipler/build -j32 --output-on-failure
```
