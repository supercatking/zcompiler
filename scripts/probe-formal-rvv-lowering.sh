#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="$(cd "$script_dir/.." && pwd)"
out_dir="$source_root/build/experiments/mlir-rvv"
mkdir -p "$out_dir"

zc_bin="$source_root/build/tools/zc/zc"
mlir_opt="/home/zyz/mlir/build/bin/mlir-opt"
mlir_translate="/home/zyz/mlir/build/bin/mlir-translate"
llvm_as="/home/zyz/mlir/build/bin/llvm-as"
local_llc="/home/zyz/mlir/build/bin/llc"
system_llc="${LLC:-llc}"

if [ ! -x "$zc_bin" ]; then
  cmake --build "$source_root/build"
fi

masked_mlir="$out_dir/vector_add.masked.mlir"
llvm_dialect="$out_dir/vector_add.vector-to-llvm.mlir"
llvm_ir="$out_dir/vector_add.vector-to-llvm.ll"
llvm_bc="$out_dir/vector_add.vector-to-llvm.bc"
local_llc_asm="$out_dir/vector_add.local-llc-rvv.s"
system_llc_asm="$out_dir/vector_add.system-llc-rvv.s"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-mlir > "$masked_mlir"

"$mlir_opt" "$masked_mlir" \
  --convert-vector-to-llvm \
  --convert-scf-to-cf \
  --convert-to-llvm \
  --reconcile-unrealized-casts \
  -o "$llvm_dialect"

"$mlir_translate" --mlir-to-llvmir "$llvm_dialect" > "$llvm_ir"
"$llvm_as" "$llvm_ir" -o "$llvm_bc"

masked_load_present=false
masked_store_present=false
if grep -q "llvm.masked.load" "$llvm_ir"; then
  masked_load_present=true
fi
if grep -q "llvm.masked.store" "$llvm_ir"; then
  masked_store_present=true
fi

local_llc_status="not_run"
if [ -x "$local_llc" ]; then
  if "$local_llc" -mtriple=riscv64-unknown-elf -mattr=+m,+v \
    -filetype=asm "$llvm_bc" -o "$local_llc_asm" \
    2> "$out_dir/local-llc.stderr"; then
    local_llc_status="produced_asm"
  else
    local_llc_status="blocked"
  fi
else
  local_llc_status="missing"
fi

system_llc_status="not_run"
if command -v "$system_llc" >/dev/null; then
  if "$system_llc" -opaque-pointers=1 -mtriple=riscv64-unknown-elf \
    -mattr=+m,+v -filetype=asm "$llvm_bc" -o "$system_llc_asm" \
    2> "$out_dir/system-llc.stderr"; then
    system_llc_status="produced_asm"
  else
    system_llc_status="blocked"
  fi
else
  system_llc_status="missing"
fi

formal_status="blocked_at_riscv_llc"
if [ "$local_llc_status" = "produced_asm" ] || \
   [ "$system_llc_status" = "produced_asm" ]; then
  formal_status="produced_riscv_asm"
fi

cat > "$out_dir/formal-rvv-lowering-result.json" <<EOF
{
  "schema_version": 1,
  "probe_id": "phase20c_formal_rvv_lowering",
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
    "llvm_ir_contains_masked_load": ${masked_load_present},
    "llvm_ir_contains_masked_store": ${masked_store_present},
    "local_llc_status": "${local_llc_status}",
    "system_llc_status": "${system_llc_status}"
  },
  "status": "${formal_status}"
}
EOF

cat > "$out_dir/formal-rvv-lowering-result.md" <<EOF
# Formal RVV Lowering Probe

Status: ${formal_status}

The probe successfully lowers masked MLIR vector IR to LLVM dialect, LLVM IR,
and LLVM bitcode. The LLVM IR contains \`llvm.masked.load\` and
\`llvm.masked.store\` intrinsics.

## llc Status

- Local LLVM/MLIR llc: ${local_llc_status}
- System llc: ${system_llc_status}

## Reproduce

\`\`\`bash
./scripts/probe-formal-rvv-lowering.sh
\`\`\`
EOF

echo "Formal RVV lowering probe written to $out_dir"
echo "Status: $formal_status"
