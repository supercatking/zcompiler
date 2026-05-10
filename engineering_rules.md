# Engineering Rules

This document records the engineering rules for all future `zcompiler`
development.

## Architecture First

- Think through the core architecture before writing code.
- Update `arch.md`, `arch_cn.md`, and related design documents before or with
  meaningful implementation changes.
- Add or update architecture diagrams, UML diagrams, and workflow diagrams when
  module relationships change.

## Modularity

- Keep lexer, parser, AST, MLIR dialect, lowering, backend, benchmark, and AI
  workflow modules separated.
- Avoid making frontend code depend directly on MLIR operation classes.
- Avoid making dialect/lowering code depend on source-language parser details.
- Prefer narrow interfaces and data ownership that is easy to reason about.

## Readability

- Prefer clear code over clever code.
- Use descriptive names for compiler stages, IR nodes, passes, and tests.
- Add comments only where the logic is not obvious.
- Keep generated text formats stable enough for golden tests.

## Testing

- Every phase should add or update tests.
- Prefer golden tests for visible compiler outputs.
- Validate generated MLIR with MLIR tools when possible.
- Validate generated LLVM IR with LLVM tools when possible.
- Validate generated RISC-V assembly with RISC-V tools when possible.

## Known Issues

- Avoid leaving known bugs unresolved.
- If a hard bug cannot be fixed immediately, record it in `known_issue.md`.
- Each known issue must include:
  - impact
  - reproduction command
  - suspected cause
  - next action

## AI-Assisted Changes

- AI may suggest compiler changes, tests, passes, and optimizations.
- AI-generated changes must pass the same tests as human-written changes.
- Non-trivial AI-generated changes should include an experiment or review
  record.

