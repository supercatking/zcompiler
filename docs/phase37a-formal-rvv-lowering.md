# Formal RVV Lowering Probe (Phase 37A)

Status: blocked_at_riscv_llc

The probe successfully lowers masked MLIR vector IR to LLVM dialect, LLVM IR,
and LLVM bitcode. The LLVM IR contains `llvm.masked.load` and
`llvm.masked.store` intrinsics.

## llc Status

- Local LLVM/MLIR llc: blocked
- System llc: blocked

## Reproduce

```bash
MLIR_BUILD=/home/zyz/mlir/build ./scripts/probe-formal-rvv-lowering.sh
```


## Interpretation

Phase 37A is complete as a repeatable probe, not as a solved formal RVV codegen
path. The MLIR vector pipeline reaches LLVM IR and bitcode with masked load/store
intrinsics. The remaining blocker is the available `llc` configuration: both the
local and system `llc` runs are currently blocked for RISC-V RVV assembly
emission. The direct RVV reference backend remains the validated executable path.

## Captured Result

```json
{
  "schema_version": 1,
  "probe_id": "phase37a_formal_rvv_lowering",
  "source_program": "examples/vector_add.zc",
  "output_directory": "build/experiments/mlir-rvv",
  "pipeline": [
    "zc --emit-mlir",
    "mlir-opt --convert-vector-to-llvm --convert-scf-to-cf --convert-to-llvm --reconcile-unrealized-casts",
    "mlir-translate --mlir-to-llvmir",
    "llvm-as",
    "llc -mtriple=riscv64-unknown-elf -mattr=+m,+v"
  ],
  "artifacts": {
    "masked_mlir": "vector_add.masked.mlir",
    "llvm_dialect": "vector_add.vector-to-llvm.mlir",
    "llvm_ir": "vector_add.vector-to-llvm.ll",
    "llvm_bitcode": "vector_add.vector-to-llvm.bc",
    "local_llc_stderr": "local-llc.stderr",
    "system_llc_stderr": "system-llc.stderr"
  },
  "observations": {
    "llvm_ir_contains_masked_load": true,
    "llvm_ir_contains_masked_store": true,
    "local_llc_status": "blocked",
    "system_llc_status": "blocked"
  },
  "status": "blocked_at_riscv_llc"
}
```
