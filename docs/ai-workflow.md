# AI-Assisted Compiler Workflow

This document defines the first AI-assisted workflow for `zcompiler`.

## Goal

Use AI to help propose compiler changes, but require every proposal to be
validated by reproducible tests, generated IR inspection, and benchmark records.

## Experiment Record Format

Each optimization experiment should record:

```text
experiment_id:
source_program:
accelerator_profile:
compiler_flags:
baseline_output:
candidate_output:
expected_behavior:
validation_commands:
result:
notes:
```

## Prompt Record Format

Each AI-assisted compiler change should record:

```text
prompt:
model:
input_files:
requested_change:
accelerator_profile:
generated_change_summary:
human_review_notes:
accepted:
```

## Safety Rules

- Do not accept AI-generated compiler changes without tests.
- Keep generated IR or assembly samples for non-trivial lowering changes.
- Prefer small compiler passes with narrow behavior.
- Compare scalar reference output before accepting vector/RVV optimizations.
- Cite the accelerator profile used for vector/RVV experiments.
- Record failed experiments; they are useful negative examples.

## Near-Term Uses

- Generate candidate peephole optimizations.
- Suggest lowering patterns from `zc` operations to MLIR dialects.
- Summarize benchmark differences.
- Create new tests for parser, lowering, LLVM IR, and RISC-V assembly.
- Propose new vector kernel forms only when paired with parser, MLIR, RVV
  assembly, correctness, and profile updates.

## Phase Roadmap

AI-assisted compiler work is planned in Phase 22 after the compiler has a more
real MLIR and backend foundation.

Before Phase 22, each phase should still keep enough records for future AI
experiments:

- source program
- accelerator profile
- compiler command
- generated MLIR / LLVM IR / assembly
- test result
- important design decision

## Workflow Diagram

```text
developer goal
  -> architecture note
  -> AI suggestion or manual design
  -> implementation patch
  -> tests and generated IR inspection
  -> benchmark or correctness record
  -> accept / reject decision
```

## Review Checklist

Before accepting an AI-assisted compiler change:

- Is the module boundary still clean?
- Are parser, AST, dialect, lowering, and backend responsibilities separated?
- Are tests added or updated?
- Is generated IR or assembly inspected when relevant?
- Is the result reproducible from one command?
- Does the change need a benchmark record?

## Record Storage

Future experiment records should live under:

```text
experiments/
  README.md
  prompts/
  results/
  benchmarks/
```

The directory now exists. The first accepted experiment is:

```text
experiments/results/vector_add_rvv_001.md
experiments/results/vector_kernel_surface_001.md
```

Prompt records live under:

```text
experiments/prompts/
```

Use `experiments/prompts/TEMPLATE.md` for future AI-assisted compiler changes.
The first prompt record is:

```text
experiments/prompts/vector_add_rvv_001.md
experiments/prompts/vector_kernel_surface_001.md
```

## Current Acceptance Standard For Vector Kernels

For Phase 25 and later vector-kernel work, an accepted compiler change must
include:

- Source example under `examples/`.
- Lexer and parser golden coverage.
- MLIR golden coverage with masked tail behavior when memory is touched.
- Direct RVV reference assembly coverage when the operation has an RVV mapping.
- Host-side correctness record or a documented executable test.
- Accelerator profile update when the supported operation set changes.

This standard was applied to `vector_add`, `vector_copy`, `vector_scale`, and
`vector_reduce_add`.

## Phase 30R Workflow Note

Masked store followed the current AI-assisted compiler workflow: define the architecture slice first, implement the minimal source/AST/MLIR/RVV path, generate golden outputs, run host correctness, run objdump checks, then run QEMU execution before committing. This phase is a good template for future RVV memory-form work because it separates semantic checks from instruction-shape checks.

## Phase 30S Workflow Note

Masked load added an architecture caveat discovered during planning: under `ta, ma`, false lanes of a masked load cannot be treated as preserved. The accepted implementation therefore uses an explicit merge with passthrough. Future AI-generated RVV proposals should state which tail/mask policy assumptions they rely on before code is accepted.
