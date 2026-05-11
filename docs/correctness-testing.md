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

## Phase 25A Vector Copy Host Harness

The second host harness is:

```text
test/correctness/vector_copy_host.py
```

It validates the same masked chunk and inactive-tail behavior for:

```zc
vector_copy c, a, n;
```

The test writes:

```text
build/correctness/vector_copy_host.json
```

## Phase 25B Vector Scale Host Harness

The third host harness is:

```text
test/correctness/vector_scale_host.py
```

It validates masked chunk behavior for:

```zc
vector_scale c, a, factor, n;
```

The MLIR check requires:

- `vector.create_mask`
- `vector.transfer_read`
- `vector.transfer_write`
- `vector.broadcast`
- `arith.muli`
- `arith.minui`

The test writes:

```text
build/correctness/vector_scale_host.json
```

## Phase 25C Vector Reduce Add Host Harness

The fourth host harness is:

```text
test/correctness/vector_reduce_add_host.py
```

It validates scalar reduction behavior for:

```zc
let sum = 0;
vector_reduce_add sum, a, n;
return sum;
```

The MLIR check requires:

- `scf.for`
- `iter_args`
- `vector.create_mask`
- `vector.transfer_read`
- `vector.reduction <add>`
- `scf.yield`
- `arith.minui`

The test writes:

```text
build/correctness/vector_reduce_add_host.json
```

## Phase 30A Vector Multiply Host Harness

The fifth host harness is:

```text
test/correctness/vector_mul_host.py
```

It validates masked chunk behavior for:

```zc
vector_mul c, a, b, n;
```

The MLIR check requires:

- `vector.create_mask`
- `vector.transfer_read`
- `vector.transfer_write`
- `arith.muli`
- `arith.minui`

The test writes:

```text
build/correctness/vector_mul_host.json
```

## Phase 30J Vector Select Greater-Than Host Harness

The sixth host harness is:

```text
test/correctness/vector_select_gt_host.py
```

It validates masked chunk behavior for:

```zc
vector_select_gt out, lhs, rhs, true_values, false_values, n;
```

The MLIR check requires:

- `vector.create_mask`
- `vector.transfer_read`
- `vector.transfer_write`
- `arith.cmpi sgt`
- `arith.select`
- `arith.minui`

The test writes:

```text
build/correctness/vector_select_gt_host.json
```

Phase 30K adds the equality variant harness:

```text
test/correctness/vector_select_eq_host.py
build/correctness/vector_select_eq_host.json
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
python3 -m json.tool build/correctness/vector_copy_host.json
python3 -m json.tool build/correctness/vector_scale_host.json
python3 -m json.tool build/correctness/vector_mul_host.json
python3 -m json.tool build/correctness/vector_reduce_add_host.json
python3 -m json.tool build/correctness/vector_select_gt_host.json
python3 -m json.tool build/correctness/vector_select_eq_host.json
python3 -m json.tool build/correctness/vector_masked_add_gt_host.json
python3 -m json.tool build/correctness/vector_masked_add_predicates_host.json
```

Phase 30K adds the equality variant harness:

```text
test/correctness/vector_select_eq_host.py
build/correctness/vector_select_eq_host.json
```


## Phase 30L Signed Select Predicate Coverage

Host correctness now covers `vector_select_lt`, `vector_select_le`,
`vector_select_gt`, `vector_select_ge`, `vector_select_eq`, and
`vector_select_ne`. QEMU runtime validation links all six signed i32 select
kernels into the RVV harness and checks both selected true and false lanes.


## Phase 30M Unsigned Select Predicate Coverage

Host correctness now also covers `vector_select_ult`, `vector_select_ule`,
`vector_select_ugt`, and `vector_select_uge`. These tests compare `i32` source
lanes as unsigned 32-bit bit patterns and the QEMU RVV harness validates the
same semantics with `vmsltu.vv` and `vmsleu.vv`.


## Phase 30O Masked Add Coverage

Host correctness now covers the first transient-mask arithmetic slice:

```zc
vector_mask_gt m0, mask_lhs, mask_rhs, n;
vector_masked_add out, a, b, m0, passthrough, n;
```

The MLIR check requires `arith.cmpi sgt`, `arith.addi`, `arith.select`, masked
vector transfers, and dynamic tail masks. The host model checks `i32` wrapping
for active mask lanes and passthrough preservation for inactive mask lanes. QEMU
runtime validation links the generated `masked_add_gt` kernel into the generated
RVV harness.

The test writes:

```text
build/correctness/vector_masked_add_gt_host.json
```


## Phase 30P Mask Predicate Family Coverage

Host correctness now covers masked add driven by all current compare mask
predicates: `lt`, `le`, `gt`, `ge`, `eq`, `ne`, `ult`, `ule`, `ugt`, and
`uge`. The generated QEMU harness links all ten `masked_add_*` functions and
checks signed, unsigned, equality, active-lane, passthrough-lane, and tail-length
behavior.

The aggregate host test writes:

```text
build/correctness/vector_masked_add_predicates_host.json
```
