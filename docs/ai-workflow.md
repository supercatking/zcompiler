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

