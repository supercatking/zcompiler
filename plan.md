# Implementation Plan

This document defines the phased implementation plan for the first `zcompiler`
toy compiler.

## Phase 0: Repository Bootstrap

Goal: create a clean GitHub project foundation.

Deliverables:

- `README.md` with final project vision.
- `arch.md` with toy compiler architecture.
- `plan.md` with phased goals.
- `.gitignore`.
- Basic directory skeleton.

Exit criteria:

- Repository can be cloned on WSL.
- Documentation explains the first compiler direction clearly.
- GitHub `main` branch has the initial project files.

## Phase 1: Build System and Tool Skeleton

Goal: build a minimal command-line compiler executable.

Deliverables:

- Top-level `CMakeLists.txt`.
- `tools/zc/zc.cpp`.
- Basic command-line options:
  - `--emit-tokens`
  - `--emit-ast`
  - `--emit-mlir`
  - `-o <file>`
- Initial CI-friendly build instructions.

Exit criteria:

- `cmake -G Ninja -S . -B build` works.
- `cmake --build build` produces `zc`.
- `zc --help` runs successfully.

## Phase 2: Lexer

Goal: convert `.zc` source text into tokens.

Deliverables:

- Token definitions.
- Lexer implementation.
- Error reporting with line and column.
- Token dump mode through `zc --emit-tokens`.
- Lexer tests.

Exit criteria:

- The compiler can tokenize the first example:

```zc
func main() -> i32 {
  let x = 1 + 2 * 3;
  return x;
}
```

## Phase 3: Parser and AST

Goal: parse tokens into a structured AST.

Deliverables:

- AST node classes.
- Recursive descent parser.
- Operator precedence parser for arithmetic expressions.
- AST dump mode through `zc --emit-ast`.
- Parser tests.

Exit criteria:

- Valid source files produce readable AST output.
- Invalid syntax produces useful errors.

## Phase 4: First MLIR Emission

Goal: emit valid MLIR from the AST.

Deliverables:

- MLIR context setup.
- First MLIR generator.
- Initial use of builtin, func, and arith dialects.
- `zc --emit-mlir`.
- MLIR output tests.

Exit criteria:

- Example source emits MLIR similar to:

```mlir
module {
  func.func @main() -> i32 {
    %c1 = arith.constant 1 : i32
    %c2 = arith.constant 2 : i32
    %c3 = arith.constant 3 : i32
    %0 = arith.muli %c2, %c3 : i32
    %1 = arith.addi %c1, %0 : i32
    return %1 : i32
  }
}
```

## Phase 5: Custom zc Dialect

Goal: introduce a project-owned MLIR dialect.

Deliverables:

- `zc` dialect definition.
- TableGen operation definitions.
- CMake integration for MLIR TableGen.
- Operations for constants, arithmetic, function, and return.
- Tests for parsing and printing `zc` dialect MLIR.

Exit criteria:

- The compiler can emit valid `zc` dialect MLIR.
- `mlir-opt` can parse and print the generated dialect when linked with the
  project plugin or tool.

## Phase 6: Lowering Passes

Goal: lower from `zc` dialect to standard MLIR dialects.

Deliverables:

- `zc` to `func` / `arith` conversion pass.
- Canonicalization patterns.
- Pass pipeline option in `zc`.
- Lowering tests.

Exit criteria:

- `zc` dialect MLIR lowers to standard MLIR.
- The lowered MLIR no longer contains `zc` operations.

## Phase 7: LLVM IR Emission

Goal: lower standard MLIR to LLVM dialect and LLVM IR.

Deliverables:

- MLIR lowering pipeline to LLVM dialect.
- `mlir-translate` compatible output.
- `zc --emit-llvm`.
- LLVM IR tests.

Exit criteria:

- Example source compiles to LLVM IR.
- The generated LLVM IR has a valid `main` returning `i32`.

## Phase 8: RISC-V Assembly

Goal: generate RISC-V assembly using LLVM backend.

Deliverables:

- `zc --emit-riscv-asm`.
- Integration with LLVM target settings.
- Initial `riscv64-linux-gnu` target triple support.
- Assembly tests.

Exit criteria:

- Example source generates RISC-V assembly.
- Return value can be validated through QEMU or assembly inspection.

## Phase 9: Control Flow

Goal: add basic control flow to prepare for real programs.

Deliverables:

- `if` / `else`.
- `while` or `for`.
- Comparison operators.
- Lowering to `scf` and then LLVM.

Exit criteria:

- Small branching and loop programs compile and run.

## Phase 10: Vector and RVV Preparation

Goal: prepare the compiler for RISC-V Vector Extension work.

Deliverables:

- Vector type syntax proposal.
- Initial vector operation design.
- Lowering path through MLIR vector dialect.
- RVV research notes and examples.

Exit criteria:

- The architecture has a concrete path from source-level vector operations to
  MLIR vector dialect and then to RISC-V RVV code generation.

## Phase 11: AI-Assisted Compiler Direction

Goal: begin connecting compiler development with AI-assisted workflows.

Deliverables:

- Pass tuning experiment format.
- Benchmark collection format.
- Prompt and evaluation notes for AI-generated optimization suggestions.
- Safety rules for accepting AI-generated compiler changes.

Exit criteria:

- The project can record optimization experiments and compare generated code or
  runtime results.

## Phase 12: Registered zc MLIR Dialect

Goal: turn the current textual `zc` dialect surface into a real registered MLIR
dialect.

Deliverables:

- TableGen-backed `ZCDialect`.
- TableGen-backed operation definitions.
- CMake integration for MLIR TableGen.
- Dialect registration in project tools.
- A small `zc-opt` style tool or equivalent parser path in `zc`.
- Dialect parsing and printing tests.

Architecture constraints:

- Keep dialect IR code separate from frontend parser and AST code.
- Do not let source-language parser logic depend on MLIR operation classes.
- Treat dialect registration as an infrastructure layer under `lib/Dialect`.

Exit criteria:

```bash
zc examples/hello.zc --emit-zc-mlir > /tmp/hello.zc.mlir
zc-opt /tmp/hello.zc.mlir
```

## Phase 13: Real MLIR Lowering Pass

Goal: lower registered `zc` dialect operations with MLIR rewrite patterns.

Deliverables:

- `ZCToStandard` conversion pass.
- Rewrite patterns for constants, arithmetic, compare, return, and function ops.
- Pass registration.
- Pass pipeline option.
- Lowering tests with `zc` input IR and expected standard MLIR output.

Architecture constraints:

- Keep lowering code under `lib/Conversion`.
- Keep lowering independent from source parser and AST.
- Prefer small patterns with one clear operation responsibility each.

Exit criteria:

```text
input: zc dialect MLIR
output: func / arith / scf MLIR
assertion: output contains no zc. operations
```

## Phase 14: AST to In-Memory MLIR

Goal: replace the main text-only MLIR emission path with real MLIR C++ API
construction.

Deliverables:

- `MLIRContext` setup.
- Dialect loading.
- `OpBuilder`-based module construction.
- AST-to-MLIR codegen that returns `mlir::ModuleOp`.
- Diagnostic handling.
- Tests proving printed MLIR comes from `ModuleOp::print`.

Architecture constraints:

- Keep AST as frontend-owned data.
- Keep MLIR construction in `lib/CodeGen` or a dedicated `lib/MLIRGen`.
- Do not make AST nodes include MLIR headers.

Exit criteria:

```bash
zc examples/hello.zc --emit-mlir > /tmp/hello.mlir
mlir-opt /tmp/hello.mlir -o /tmp/hello.checked.mlir
```

## Phase 15: MLIR to LLVM Dialect and LLVM IR

Goal: use the MLIR lowering pipeline instead of hand-written LLVM IR text for
the main LLVM path.

Deliverables:

- Standard MLIR to LLVM dialect lowering pipeline.
- LLVM dialect to LLVM IR translation path.
- `--emit-llvm` backed by MLIR/LLVM infrastructure.
- Tests with `llvm-as`.

Architecture constraints:

- Keep textual LLVM IR fallback only for debugging if needed.
- Prefer official MLIR conversion passes over custom string generation.

Exit criteria:

```bash
zc examples/hello.zc --emit-llvm > /tmp/hello.ll
llvm-as /tmp/hello.ll -o /tmp/hello.bc
```

## Phase 16: LLVM RISC-V Backend Integration

Goal: generate RISC-V assembly through LLVM's RISC-V backend.

Deliverables:

- RISC-V target initialization.
- Target triple configuration.
- LLVM IR to RISC-V assembly path.
- Optional object file emission.
- Tests with `riscv64-linux-gnu-as`.

Architecture constraints:

- Keep the current hand-written RISC-V emitter as a debug/reference backend
  until the LLVM backend path is stable.
- Put backend-specific code behind a clear target interface.

Exit criteria:

```bash
zc examples/hello.zc --emit-riscv-asm > /tmp/hello.s
riscv64-linux-gnu-as /tmp/hello.s -o /tmp/hello.o
```

## Phase 17: Functions and Memory

Goal: extend the source language enough to express simple kernels.

Design note: the first implementation slice is documented in
`docs/phase17-functions-memory.md`. It implements parameters, calls, and
straight-line assignment first, then extends into explicit array/memref
load/store syntax.

Phase 17B extends that design with `ptr<i32>` parameters plus `load a[i]` and
`store c[i] = value;` so vector work has a real scalar memory baseline.

Deliverables:

- Function parameters.
- Function calls.
- Mutable variables or assignment.
- Basic pointer/memory model.
- Array or memref-like load/store syntax.
- Parser, AST, MLIR, LLVM IR, and RISC-V tests.

Architecture constraints:

- Keep type representation explicit and reusable.
- Separate semantic checks from parsing where practical.

Exit criteria:

```text
The compiler can represent and compile a simple array-style kernel skeleton.
```

## Phase 18: Vector Syntax and AST

Goal: add source-level vector concepts.

Design note: Phase 18A is documented in
`docs/phase18-vector-syntax.md`. It starts with a target-independent
`vector_add c, a, b, n;` statement over `ptr<i32>` buffers.

Deliverables:

- Vector type syntax.
- Vector literal or vector value representation.
- Vector load/store syntax.
- Vector arithmetic syntax.
- Vector AST nodes.
- Parser and AST dump tests.

Architecture constraints:

- Do not tie source vector syntax directly to RVV instruction names.
- Keep vector operations target-independent at the source and AST layers.

Exit criteria:

```zc
func main() -> i32 {
  let c = vector.add(a, b);
  return 0;
}
```

The compiler can parse and dump vector AST.

## Phase 19: Lower Vector Ops to MLIR Vector Dialect

Goal: lower target-independent vector source operations to MLIR vector dialect.

Design note: Phase 19A/19B is documented in
`docs/phase19-vector-mlir.md`. It lowers `vector_add` to a fixed
`vector<4xi32>` MLIR loop and uses `vector.create_mask` plus masked transfer
operations for tail-safe memory access.

Deliverables:

- `zc.vector_*` operation design.
- Lowering to MLIR vector dialect.
- Tests that inspect generated vector dialect MLIR.

Architecture constraints:

- Use MLIR vector dialect as the main abstraction before RVV-specific lowering.
- Keep RVV-specific choices out of source parsing.

Exit criteria:

```text
Generated MLIR contains vector dialect operations for vector examples.
```

## Phase 20: RVV Lowering

Goal: generate RVV-oriented LLVM IR or assembly.

Design note: Phase 20A is documented in
`docs/phase20-rvv-lowering.md`. It starts with a direct RVV reference assembly
path for `vector_add`. Phase 20C probes the formal MLIR/LLVM lowering route
and records the current RISC-V `llc` toolchain blocker in
`docs/phase20c-formal-rvv-lowering.md`.

Deliverables:

- RVV target feature configuration.
- Lowering path toward RVV intrinsics or RVV assembly.
- Tests for `vsetvli`, vector load/store, vector arithmetic, and reduction.

Architecture constraints:

- Prefer backend-supported lowering when available.
- Use direct RVV assembly only when it is clearly documented as a temporary
  reference path.

Exit criteria:

```text
Generated output contains expected RVV instruction families.
```

## Phase 21: Benchmark and Accelerator Profile

Goal: define the target accelerator assumptions and measure scalar versus
vector paths.

Phase 21A starts with `benchmarks/vector_add/`, which generates reference C RVV
artifacts and zcompiler RVV artifacts side by side. Phase 21C adds a scalar C
baseline and records scalar-vs-vector comparison metadata.

Deliverables:

- Accelerator profile file.
- Benchmark runner.
- Scalar baseline outputs.
- Vector candidate outputs.
- Correctness comparison.
- Performance record format.

Architecture constraints:

- Benchmarks must be reproducible.
- Benchmark metadata should be machine-readable.

Exit criteria:

```text
At least one scalar-vs-vector experiment can be recorded and reproduced.
```

## Phase 22: AI-Assisted Optimization Loop

Goal: implement the workflow for AI-suggested compiler changes.

Phase 22B adds prompt records under `experiments/prompts/`, including a template
and the first accepted vector-add/RVV prompt record.

Deliverables:

- Experiment schema.
- Prompt schema.
- Result parser.
- Candidate optimization proposal format.
- Human review checklist.
- Regression test integration.

Architecture constraints:

- AI suggestions must not bypass tests.
- Every accepted AI-generated compiler change needs an experiment record or a
  clear test record.

Exit criteria:

```text
One AI-assisted compiler proposal can be recorded, tested, compared, and marked
accepted or rejected.
```

## Phase 23: Machine-Readable Accelerator Profile

Goal: make RVV target assumptions explicit and reusable by compiler tests,
benchmarks, and AI experiment records.

Deliverables:

- `profiles/rvv-default.json`.
- Documentation for target triple, `march`, `mabi`, SEW/LMUL policy, and
  tail/mask policy.
- Benchmark metadata that cites the active profile.

Exit criteria:

```text
Generated benchmark and correctness records cite the active accelerator profile.
```

## Phase 24: Correctness Harness

Goal: validate vector semantics independently from golden text diffs.

Deliverables:

- Host-side scalar reference models.
- Host-side masked vector chunk models.
- Generated JSON correctness records.
- CTest integration.

Exit criteria:

```text
Vector operations have semantic checks for normal and tail lengths.
```

## Phase 25: Vector Kernel Surface Expansion

Goal: grow from one vector operation into a small family of kernels while
preserving clean module boundaries.

Completed slices:

- Phase 25A: `vector_copy c, a, n;`
- Phase 25B: `vector_scale c, a, factor, n;`
- Phase 25C: `vector_reduce_add sum, a, n;`

Deliverables:

- Source examples for each kernel.
- Lexer, parser, AST, MLIR, RVV assembly, assembler, objdump, and correctness
  coverage for each kernel.
- Shared MLIR masked vector loop/read/write helpers.
- Accelerator profile updates whenever the supported operation set changes.

Architecture constraints:

- Keep source syntax target-independent.
- Keep RVV instruction selection in target/lowering code.
- Reuse helper APIs for common vector memory/tail behavior.

Exit criteria:

```text
At least three vector kernels compile through MLIR vector dialect and direct
RVV reference assembly, with semantic tail checks.
```

## Phase 26: Formal MLIR RVV Lowering Unblock

Goal: align the local LLVM/MLIR toolchain so masked vector dialect IR can lower
through LLVM to RISC-V RVV assembly without the temporary direct assembly path.

Deliverables:

- Toolchain alignment plan for local LLVM 23 MLIR and RISC-V-enabled `llc`.
- Repeatable RVV toolchain diagnostic script.
- Dry-run/configure/build script for a separate RISCV-enabled LLVM build.
- Re-run of `scripts/probe-formal-rvv-lowering.sh`.
- Updated formal lowering notes and tests if the blocker is removed.

Exit criteria:

```text
The formal MLIR vector-to-LLVM-to-RISC-V path reaches RVV assembly in the local
environment, or the remaining blocker is precisely documented.
```

## Phase 27: AI-Guided Kernel Optimization Records

Goal: use the AI workflow to compare kernel-lowering alternatives under the
active accelerator profile.

Phase 27A adds a profile-aware experiment and prompt record for the vector
kernel surface.

Deliverables:

- Prompt/result records for at least one kernel-lowering proposal.
- Profile-aware comparison metadata.
- Test and benchmark references for accepted or rejected changes.

Exit criteria:

```text
One vector-kernel optimization proposal is recorded with profile, tests,
generated IR/assembly, and accept/reject result.
```

## Phase 28: Executable RVV Demo and Runtime Output

Goal: make zcompiler-generated RISC-V64/RVV programs observable and executable
under QEMU.

Completed slices:

- Phase 28A: current capability demo in `examples/complex_vector_pipeline.zc`.
- Phase 28B: built-in `print_i32 expr;` RISC-V runtime output.
- Phase 28C: `qemu-riscv64` CTest execution validation.

Exit criteria:

```text
A zcompiler-generated RISC-V64 binary can print a value and RVV kernels can run
under QEMU with checked results.
```

## Phase 29: RVV 1.0 Compliance Tracking

Goal: turn the current RVV subset into a tracked RVV 1.0 compatibility program.

Phase 29A establishes the compliance baseline:

- `profiles/rvv-default.json` records RVV spec version `1.0`.
- `docs/rvv-1.0-compliance.md` records supported features, gaps, and acceptance
  rules.
- QEMU validation covers lengths `0`, `1`, `2`, `3`, `4`, `5`, `7`, `8`, `9`,
  `16`, and `17`.

Planned slices:

- Phase 29B: widen source/kernel contracts beyond the current `i32`-only
  surface.
- Phase 29C: introduce LMUL policy and tests beyond the current `m1` subset.
- Phase 29D: add negative-value and overflow-defined test cases for integer
  kernels where semantics are well defined.

Exit criteria:

```text
Every supported RVV feature has a profile entry, compliance matrix row,
assembly/objdump check, and QEMU execution check.
```

## Phase 30: Broaden RVV Kernel Surface

Goal: grow the RVV 1.0 compatible subset one small kernel family at a time while
keeping every feature covered by lexer/parser/codegen tests, MLIR validation,
objdump checks, profile entries, and QEMU execution.

Completed slices:

- Phase 30A: `vector_mul c, a, b, n;` elementwise multiply.
- Phase 30B: QEMU execution manifest for the runtime validation matrix.
- Phase 30C: validated QEMU kernel descriptors in the execution manifest.
- Phase 30D: signed `i32` runtime coverage and integer semantics note.
- Phase 30E: wrapping `i32` bit-pattern checks for current RVV kernels.
- Phase 30F: scalar source `i32` wrapping semantics for direct RISC-V output.
- Phase 30G: generated QEMU C harness from validated manifest data.
- Phase 30H: split QEMU check rendering into per-family generator units.

Planned slices:

- Phase 30I: compare/select source syntax and RVV predicate lowering design.
- Phase 29B: source element-width contract for RVV kernels.

Exit criteria:

```text
New RVV kernels can be added with minimal harness duplication and each supported
operation has source syntax, MLIR lowering, direct RVV assembly, objdump checks,
and QEMU runtime validation.
```

## Engineering Rules For All Future Phases

- Think through the core architecture first and document it before coding.
- Keep code structured, modular, readable, and extensible.
- Reduce coupling between lexer, parser, AST, MLIR dialect, lowering, target
  backend, benchmark, and AI workflow layers.
- Add useful comments for non-obvious logic, but avoid noisy comments.
- Update architecture diagrams, UML diagrams, and workflow diagrams whenever the
  architecture changes meaningfully.
- Add test cases with each phase.
- Prefer complete validation. If a difficult bug cannot be fixed immediately,
  document it in `known_issue.md` with impact, reproduction, and next action.

## Immediate Next Tasks

The next implementation steps after Phase 29A compliance baseline:

1. Start compare/select source syntax and RVV predicate lowering design.
2. Start Phase 29B by deciding whether the source language exposes element
   width in syntax or through typed buffers first.
4. Add compare/select kernels so vector predicates can become first-class.
5. Run `./scripts/prepare-riscv-llvm-build.sh --configure` or `--build` when
   ready for the larger same-version RISC-V-enabled LLVM/MLIR build.
