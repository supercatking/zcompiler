#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
usage: scripts/prepare-riscv-llvm-build.sh [--dry-run|--configure|--build]

Modes:
  --dry-run     Write the build plan only. This is the default.
  --configure   Run CMake configure for a separate RISCV-enabled LLVM build.
  --build       Run configure, then build llc/mlir-opt/mlir-translate/llvm-as.

Environment overrides:
  LLVM_SOURCE_DIR       default: /home/zyz/mlir/llvm-project/llvm
  LLVM_RISCV_BUILD      default: /home/zyz/mlir/build-riscv
  LLVM_ENABLE_PROJECTS  default: mlir
  LLVM_TARGETS_TO_BUILD default: X86;RISCV
  CMAKE_BUILD_TYPE      default: Release
  LLVM_BUILD_TARGETS    default: llc;mlir-opt;mlir-translate;llvm-as
EOF
}

mode="dry-run"
if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
  usage
  exit 0
elif [ "${1:-}" = "--dry-run" ] || [ $# -eq 0 ]; then
  mode="dry-run"
elif [ "${1:-}" = "--configure" ]; then
  mode="configure"
elif [ "${1:-}" = "--build" ]; then
  mode="build"
else
  usage >&2
  exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="$(cd "$script_dir/.." && pwd)"
out_dir="$source_root/build/experiments/rvv-toolchain"
mkdir -p "$out_dir"

llvm_source_dir="${LLVM_SOURCE_DIR:-/home/zyz/mlir/llvm-project/llvm}"
llvm_riscv_build="${LLVM_RISCV_BUILD:-/home/zyz/mlir/build-riscv}"
llvm_enable_projects="${LLVM_ENABLE_PROJECTS:-mlir}"
llvm_targets_to_build="${LLVM_TARGETS_TO_BUILD:-X86;RISCV}"
cmake_build_type="${CMAKE_BUILD_TYPE:-Release}"
llvm_build_targets="${LLVM_BUILD_TARGETS:-llc;mlir-opt;mlir-translate;llvm-as}"

json_file="$out_dir/riscv-llvm-build-plan.json"
md_file="$out_dir/riscv-llvm-build-plan.md"

if [ ! -d "$llvm_source_dir" ]; then
  echo "LLVM source directory does not exist: $llvm_source_dir" >&2
  exit 1
fi

IFS=';' read -r -a build_targets <<< "$llvm_build_targets"

configure_cmd=(
  cmake
  -G Ninja
  -S "$llvm_source_dir"
  -B "$llvm_riscv_build"
  -DLLVM_ENABLE_PROJECTS="$llvm_enable_projects"
  -DLLVM_TARGETS_TO_BUILD="$llvm_targets_to_build"
  -DCMAKE_BUILD_TYPE="$cmake_build_type"
)

build_cmd=(
  cmake
  --build "$llvm_riscv_build"
  --target "${build_targets[@]}"
)

shell_join() {
  local out=""
  local arg
  for arg in "$@"; do
    printf -v quoted '%q' "$arg"
    out+="$quoted "
  done
  printf '%s' "${out% }"
}

json_escape() {
  sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

configure_command="$(shell_join "${configure_cmd[@]}")"
build_command="$(shell_join "${build_cmd[@]}")"

cat > "$json_file" <<EOF
{
  "schema_version": 1,
  "plan_id": "phase26b_riscv_llvm_build",
  "mode": "${mode}",
  "llvm_source_dir": "$(printf '%s' "$llvm_source_dir" | json_escape)",
  "llvm_riscv_build": "$(printf '%s' "$llvm_riscv_build" | json_escape)",
  "cmake_build_type": "$(printf '%s' "$cmake_build_type" | json_escape)",
  "llvm_enable_projects": "$(printf '%s' "$llvm_enable_projects" | json_escape)",
  "llvm_targets_to_build": "$(printf '%s' "$llvm_targets_to_build" | json_escape)",
  "llvm_build_targets": "$(printf '%s' "$llvm_build_targets" | json_escape)",
  "configure_command": "$(printf '%s' "$configure_command" | json_escape)",
  "build_command": "$(printf '%s' "$build_command" | json_escape)",
  "post_build_checks": [
    "MLIR_BUILD=$(printf '%s' "$llvm_riscv_build" | json_escape) ./scripts/check-rvv-toolchain.sh",
    "MLIR_BUILD=$(printf '%s' "$llvm_riscv_build" | json_escape) ./scripts/probe-formal-rvv-lowering.sh"
  ]
}
EOF

cat > "$md_file" <<EOF
# RISCV LLVM Build Plan

Mode: ${mode}

## Configure

\`\`\`bash
${configure_command}
\`\`\`

## Build

\`\`\`bash
${build_command}
\`\`\`

## Post-Build Checks

\`\`\`bash
MLIR_BUILD=${llvm_riscv_build} ./scripts/check-rvv-toolchain.sh
MLIR_BUILD=${llvm_riscv_build} ./scripts/probe-formal-rvv-lowering.sh
\`\`\`
EOF

if [ "$mode" = "configure" ] || [ "$mode" = "build" ]; then
  "${configure_cmd[@]}"
fi

if [ "$mode" = "build" ]; then
  "${build_cmd[@]}"
fi

echo "RISCV LLVM build plan written to $out_dir"
echo "Mode: $mode"
