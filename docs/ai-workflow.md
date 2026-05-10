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
generated_change_summary:
human_review_notes:
accepted:
```

## Safety Rules

- Do not accept AI-generated compiler changes without tests.
- Keep generated IR or assembly samples for non-trivial lowering changes.
- Prefer small compiler passes with narrow behavior.
- Compare scalar reference output before accepting vector/RVV optimizations.
- Record failed experiments; they are useful negative examples.

## Near-Term Uses

- Generate candidate peephole optimizations.
- Suggest lowering patterns from `zc` operations to MLIR dialects.
- Summarize benchmark differences.
- Create new tests for parser, lowering, LLVM IR, and RISC-V assembly.

## Phase Roadmap

AI-assisted compiler work is planned in Phase 22 after the compiler has a more
real MLIR and backend foundation.

Before Phase 22, each phase should still keep enough records for future AI
experiments:

- source program
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
```

Prompt records live under:

```text
experiments/prompts/
```

Use `experiments/prompts/TEMPLATE.md` for future AI-assisted compiler changes.
The first prompt record is:

```text
experiments/prompts/vector_add_rvv_001.md
```
