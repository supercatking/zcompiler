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
reference_sources
output_directory
tools
required_instructions
artifacts
instruction_counts
comparisons
status
```

Human-readable summaries should be emitted as `result.md`.

Schema version 2 adds scalar-vs-vector comparison records. For vector-add this
means the benchmark records:

- Scalar C baseline artifacts.
- Hand-written RVV C reference artifacts.
- zcompiler RVV artifacts.
- Scalar RVV-instruction absence.
- Required RVV-instruction presence for vector outputs.
- Object-size deltas for zcompiler vs scalar and zcompiler vs reference RVV.

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
