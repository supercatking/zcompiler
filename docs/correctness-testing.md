# Correctness Testing

Correctness tests check semantics, not just generated text.

## Phase 24A Vector Add Host Harness

The first correctness harness is:

```text
test/correctness/vector_add_host.py
```

It runs on the host and validates the current `vector_add` lowering contract:

- `zc examples/vector_add.zc --emit-mlir` must contain masked vector lowering
  features:
  - `vector.create_mask`
  - `vector.transfer_read`
  - `vector.transfer_write`
  - `arith.minui`
- The host model executes scalar vector add and the compiler's masked
  `vector<4xi32>` chunk semantics for lengths:
  - `0`
  - `1`
  - `3`
  - `4`
  - `5`
  - `7`
  - `16`
  - `17`
- The test verifies that active lanes match the scalar result and inactive tail
  lanes are not modified.

The test writes a generated record to:

```text
build/correctness/vector_add_host.json
```

## Why Host-Side First

The current WSL environment does not provide `qemu-riscv64`, so Phase 24A starts
with a host-side semantic harness. Once an emulator is available, this should be
extended with a RISC-V executable test that links generated or reference RVV
code into a runnable program.

## Run

```bash
ctest --test-dir build --output-on-failure
python3 -m json.tool build/correctness/vector_add_host.json
```
