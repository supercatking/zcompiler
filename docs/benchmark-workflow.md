# Benchmark Workflow

This document defines the benchmark artifact workflow for `zcompiler`.

## Goals

- Keep benchmark generation reproducible.
- Store source inputs in the repository.
- Store generated artifacts under `build/`.
- Emit machine-readable metadata for later AI-assisted analysis.

## Result Schema

Benchmark scripts should emit `result.json` with:

```text
schema_version
benchmark_id
source_program
reference_source
output_directory
tools
required_instructions
artifacts
instruction_counts
status
```

Human-readable summaries should be emitted as `result.md`.

## Current Benchmarks

```text
benchmarks/vector_add/
```

Run:

```bash
./benchmarks/vector_add/run.sh
```

Outputs:

```text
build/benchmarks/vector_add/result.json
build/benchmarks/vector_add/result.md
```
