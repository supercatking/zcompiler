#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="$(cd "$script_dir/.." && pwd)"
out_dir="$source_root/build/experiments/rvv-toolchain"
mkdir -p "$out_dir"

mlir_build="${MLIR_BUILD:-/home/zyz/mlir/build}"
cmake_cache="$mlir_build/CMakeCache.txt"
mlir_opt="$mlir_build/bin/mlir-opt"
mlir_translate="$mlir_build/bin/mlir-translate"
local_llc="$mlir_build/bin/llc"
system_llc="${LLC:-$(command -v llc || true)}"
riscv_as="$(command -v riscv64-linux-gnu-as || true)"
riscv_objdump="$(command -v riscv64-linux-gnu-objdump || true)"

json_file="$out_dir/rvv-toolchain-diagnostic.json"
md_file="$out_dir/rvv-toolchain-diagnostic.md"

json_escape() {
  sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

first_version_line() {
  local tool="$1"
  if [ -n "$tool" ] && [ -x "$tool" ]; then
    local output
    output="$("$tool" --version 2>/dev/null | sed -n '1,8p')"
    local version_line
    version_line="$(printf '%s\n' "$output" | grep -i 'version' | head -n 1 || true)"
    if [ -n "$version_line" ]; then
      printf '%s\n' "$version_line"
    else
      printf '%s\n' "$output" | sed -n '1p'
    fi
  else
    echo "missing"
  fi
}

first_output_line() {
  local tool="$1"
  if [ -n "$tool" ] && [ -x "$tool" ]; then
    "$tool" --version 2>/dev/null | sed -n '1p'
  else
    echo "missing"
  fi
}

extract_llvm_major() {
  sed -n 's/.*LLVM version \([0-9][0-9]*\).*/\1/p; s/.*LLVM \([0-9][0-9]*\)\..*/\1/p' |
    sed -n '1p'
}

has_registered_target() {
  local tool="$1"
  local target="$2"
  if [ -n "$tool" ] && [ -x "$tool" ]; then
    "$tool" --version 2>/dev/null | grep -qi "$target"
  else
    return 1
  fi
}

cmake_value() {
  local key="$1"
  if [ -f "$cmake_cache" ]; then
    grep "^$key:" "$cmake_cache" | tail -n 1 | sed 's/.*=//'
  fi
}

bool_text() {
  if "$@"; then
    echo "true"
  else
    echo "false"
  fi
}

mlir_opt_version="$(first_version_line "$mlir_opt")"
mlir_translate_version="$(first_version_line "$mlir_translate")"
local_llc_version="$(first_version_line "$local_llc")"
system_llc_version="$(first_version_line "$system_llc")"
riscv_as_version="$(first_output_line "$riscv_as")"
riscv_objdump_version="$(first_output_line "$riscv_objdump")"

mlir_major="$(printf '%s\n' "$mlir_opt_version" | extract_llvm_major)"
local_llc_major="$(printf '%s\n' "$local_llc_version" | extract_llvm_major)"
system_llc_major="$(printf '%s\n' "$system_llc_version" | extract_llvm_major)"

local_llc_has_riscv="$(bool_text has_registered_target "$local_llc" "riscv64")"
system_llc_has_riscv="$(bool_text has_registered_target "$system_llc" "riscv64")"

llvm_targets_to_build="$(cmake_value LLVM_TARGETS_TO_BUILD)"
llvm_default_target="$(cmake_value LLVM_DEFAULT_TARGET_TRIPLE)"
llvm_experimental_targets="$(cmake_value LLVM_EXPERIMENTAL_TARGETS_TO_BUILD)"

local_llc_status="missing"
if [ -x "$local_llc" ]; then
  if [ "$local_llc_has_riscv" = "true" ]; then
    if [ -n "$mlir_major" ] && [ "$mlir_major" = "$local_llc_major" ]; then
      local_llc_status="ready_same_version_riscv_target"
    else
      local_llc_status="has_riscv_but_version_unknown"
    fi
  else
    local_llc_status="missing_riscv_target"
  fi
fi

system_llc_status="missing"
if [ -n "$system_llc" ] && [ -x "$system_llc" ]; then
  if [ "$system_llc_has_riscv" = "true" ]; then
    if [ -n "$mlir_major" ] && [ "$mlir_major" = "$system_llc_major" ]; then
      system_llc_status="ready_same_version_riscv_target"
    else
      system_llc_status="riscv_target_but_version_mismatch"
    fi
  else
    system_llc_status="missing_riscv_target"
  fi
fi

overall_status="blocked_local_llc_missing_riscv_target"
if [ "$local_llc_status" = "ready_same_version_riscv_target" ]; then
  overall_status="ready_to_rerun_formal_rvv_probe"
elif [ "$system_llc_status" = "ready_same_version_riscv_target" ]; then
  overall_status="ready_with_system_llc"
fi

recommendation="Configure a same-version LLVM/MLIR build with RISCV enabled, preferably in /home/zyz/mlir/build-riscv, then re-run scripts/probe-formal-rvv-lowering.sh."

cat > "$json_file" <<EOF
{
  "schema_version": 1,
  "diagnostic_id": "phase26a_rvv_toolchain",
  "mlir_build": "$(printf '%s' "$mlir_build" | json_escape)",
  "cmake_cache": {
    "path": "$(printf '%s' "$cmake_cache" | json_escape)",
    "llvm_targets_to_build": "$(printf '%s' "${llvm_targets_to_build:-unknown}" | json_escape)",
    "llvm_experimental_targets_to_build": "$(printf '%s' "${llvm_experimental_targets:-unknown}" | json_escape)",
    "llvm_default_target_triple": "$(printf '%s' "${llvm_default_target:-unknown}" | json_escape)"
  },
  "tools": {
    "mlir_opt": {
      "path": "$(printf '%s' "$mlir_opt" | json_escape)",
      "version": "$(printf '%s' "$mlir_opt_version" | json_escape)",
      "llvm_major": "$(printf '%s' "${mlir_major:-unknown}" | json_escape)"
    },
    "mlir_translate": {
      "path": "$(printf '%s' "$mlir_translate" | json_escape)",
      "version": "$(printf '%s' "$mlir_translate_version" | json_escape)"
    },
    "local_llc": {
      "path": "$(printf '%s' "$local_llc" | json_escape)",
      "version": "$(printf '%s' "$local_llc_version" | json_escape)",
      "llvm_major": "$(printf '%s' "${local_llc_major:-unknown}" | json_escape)",
      "has_riscv64_target": ${local_llc_has_riscv},
      "status": "${local_llc_status}"
    },
    "system_llc": {
      "path": "$(printf '%s' "${system_llc:-missing}" | json_escape)",
      "version": "$(printf '%s' "$system_llc_version" | json_escape)",
      "llvm_major": "$(printf '%s' "${system_llc_major:-unknown}" | json_escape)",
      "has_riscv64_target": ${system_llc_has_riscv},
      "status": "${system_llc_status}"
    },
    "riscv_assembler": {
      "path": "$(printf '%s' "${riscv_as:-missing}" | json_escape)",
      "version": "$(printf '%s' "$riscv_as_version" | json_escape)"
    },
    "riscv_objdump": {
      "path": "$(printf '%s' "${riscv_objdump:-missing}" | json_escape)",
      "version": "$(printf '%s' "$riscv_objdump_version" | json_escape)"
    }
  },
  "overall_status": "${overall_status}",
  "recommendation": "$(printf '%s' "$recommendation" | json_escape)"
}
EOF

cat > "$md_file" <<EOF
# RVV Toolchain Diagnostic

Status: ${overall_status}

## Key Findings

- MLIR tools come from: \`${mlir_build}\`
- MLIR major version: \`${mlir_major:-unknown}\`
- Local \`llc\` status: \`${local_llc_status}\`
- Local \`llc\` has RISC-V target: \`${local_llc_has_riscv}\`
- System \`llc\` status: \`${system_llc_status}\`
- System \`llc\` has RISC-V target: \`${system_llc_has_riscv}\`
- CMake \`LLVM_TARGETS_TO_BUILD\`: \`${llvm_targets_to_build:-unknown}\`

## Recommendation

${recommendation}

Suggested separate build directory:

\`\`\`bash
cmake -G Ninja -S /home/zyz/mlir/llvm-project/llvm \\
  -B /home/zyz/mlir/build-riscv \\
  -DLLVM_ENABLE_PROJECTS=mlir \\
  -DLLVM_TARGETS_TO_BUILD="X86;RISCV" \\
  -DCMAKE_BUILD_TYPE=Release

cmake --build /home/zyz/mlir/build-riscv --target llc mlir-opt mlir-translate llvm-as
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/check-rvv-toolchain.sh
MLIR_BUILD=/home/zyz/mlir/build-riscv ./scripts/probe-formal-rvv-lowering.sh
\`\`\`
EOF

echo "RVV toolchain diagnostic written to $out_dir"
echo "Status: $overall_status"
