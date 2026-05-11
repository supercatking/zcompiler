# Experiments

This directory stores reproducible AI-assisted compiler experiment records.

Each record should explain:

- the compiler goal
- the source program
- the accelerator profile, when target behavior matters
- the generated IR or assembly artifacts
- validation commands
- result and acceptance decision

Records are useful even when an experiment fails, because they preserve design
constraints and prevent repeating the same mistake.

Accepted records:

```text
experiments/results/vector_add_rvv_001.md
experiments/results/vector_kernel_surface_001.md
```
