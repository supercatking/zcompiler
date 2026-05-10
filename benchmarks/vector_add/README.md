# Vector Add Benchmark Artifacts

This directory records the first reproducible artifact workflow for RVV vector
add.

It compares two outputs:

- `reference_rvv.c`: hand-written C using RISC-V vector intrinsics.
- `examples/vector_add.zc`: zcompiler source lowered to RVV reference assembly.

## Run

```bash
./benchmarks/vector_add/run.sh
```

The script writes generated artifacts under:

```text
build/benchmarks/vector_add/
```

Expected artifacts:

- `reference_rvv.s`
- `reference_rvv.o`
- `reference_rvv.objdump`
- `zcompiler_vector_add.s`
- `zcompiler_vector_add.o`
- `zcompiler_vector_add.objdump`
- `result.json`
- `result.md`

## Current Purpose

This is not a performance benchmark yet. Phase 21A records reproducible
compiler artifacts and checks that both the reference C and zcompiler output
contain RVV instruction families.

Phase 21B adds machine-readable `result.json` metadata and a human-readable
`result.md` summary. These files are generated into the build directory and are
not checked in as golden outputs.
