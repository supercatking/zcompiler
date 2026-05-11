#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
qemu_bin="${ZCOMPILER_QEMU_RISCV64:-/home/qemu/qemu/build-riscv64-user/qemu-riscv64}"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
manifest="$source_root/test/qemu/rvv_execution_manifest.json"
manifest_env="$tmp_dir/qemu_manifest.env"

python3 "$source_root/test/qemu/manifest.py" "$manifest" \
  --emit-shell-env "$manifest_env"
# shellcheck disable=SC1090
. "$manifest_env"

if [ ! -x "$qemu_bin" ]; then
  echo "qemu-riscv64 validation skipped: $qemu_bin is not executable"
  exit 0
fi

if ! command -v riscv64-linux-gnu-gcc >/dev/null; then
  echo "qemu-riscv64 validation skipped: riscv64-linux-gnu-gcc was not found"
  exit 0
fi

"$zc_bin" "$source_root/examples/print_i32.zc" --emit-riscv-asm \
  > "$tmp_dir/print_i32.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/print_i32.s" -o "$tmp_dir/print_i32"

set +e
print_output="$("$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/print_i32")"
print_status="$?"
set -e

if [ "$print_output" != "$print_expected_stdout" ]; then
  echo "expected print_i32 stdout to be $print_expected_stdout, got: $print_output" >&2
  exit 1
fi

if [ "$print_status" -ne "$print_expected_status" ]; then
  echo "expected print_i32 exit status $print_expected_status, got: $print_status" >&2
  exit 1
fi

"$zc_bin" "$source_root/$scalar_wrap_source" --emit-riscv-asm \
  > "$tmp_dir/scalar_i32_wrap.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/scalar_i32_wrap.s" -o "$tmp_dir/scalar_i32_wrap"

set +e
scalar_wrap_output="$("$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/scalar_i32_wrap")"
scalar_wrap_status="$?"
set -e

if [ "$scalar_wrap_output" != "$scalar_wrap_expected_stdout" ]; then
  echo "expected scalar_i32_wrap stdout:" >&2
  printf '%s\n' "$scalar_wrap_expected_stdout" >&2
  echo "got:" >&2
  printf '%s\n' "$scalar_wrap_output" >&2
  exit 1
fi

if [ "$scalar_wrap_status" -ne "$scalar_wrap_expected_status" ]; then
  echo "expected scalar_i32_wrap exit status $scalar_wrap_expected_status, got: $scalar_wrap_status" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" \
  --emit-riscv-asm > "$tmp_dir/complex_vector_pipeline.s"
"$zc_bin" "$source_root/examples/vector_mul.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_mul.s"
for predicate in lt le gt ge eq ne; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" \
    --emit-riscv-asm > "$tmp_dir/vector_select_${predicate}.s"
done
python3 "$source_root/test/qemu/harness.py" "$manifest" \
  "$tmp_dir/complex_vector_pipeline_harness.c"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/complex_vector_pipeline.s" \
  "$tmp_dir/vector_mul.s" \
  "$tmp_dir/vector_select_lt.s" \
  "$tmp_dir/vector_select_le.s" \
  "$tmp_dir/vector_select_gt.s" \
  "$tmp_dir/vector_select_ge.s" \
  "$tmp_dir/vector_select_eq.s" \
  "$tmp_dir/vector_select_ne.s" \
  "$tmp_dir/complex_vector_pipeline_harness.c" \
  -o "$tmp_dir/complex_vector_pipeline"

"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/complex_vector_pipeline"

echo "qemu-riscv64 validation passed"
