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
masked_add_sources=()
for predicate in lt le gt ge eq ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_masked_add_${predicate}.zc" \
    --emit-riscv-asm > "$tmp_dir/vector_masked_add_${predicate}.s"
  masked_add_sources+=("$tmp_dir/vector_masked_add_${predicate}.s")
done
for op in sub mul; do
  "$zc_bin" "$source_root/examples/vector_masked_${op}_gt.zc" \
    --emit-riscv-asm > "$tmp_dir/vector_masked_${op}_gt.s"
done
"$zc_bin" "$source_root/examples/vector_masked_store_gt.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_masked_store_gt.s"
"$zc_bin" "$source_root/examples/vector_masked_load_gt.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_masked_load_gt.s"
for predicate in lt le gt ge eq ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" \
    --emit-riscv-asm > "$tmp_dir/vector_select_${predicate}.s"
done
python3 "$source_root/test/qemu/harness.py" "$manifest" \
  "$tmp_dir/complex_vector_pipeline_harness.c"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/complex_vector_pipeline.s" \
  "$tmp_dir/vector_mul.s" \
  "${masked_add_sources[@]}" \
  "$tmp_dir/vector_masked_sub_gt.s" \
  "$tmp_dir/vector_masked_mul_gt.s" \
  "$tmp_dir/vector_masked_store_gt.s" \
  "$tmp_dir/vector_masked_load_gt.s" \
  "$tmp_dir/vector_select_lt.s" \
  "$tmp_dir/vector_select_le.s" \
  "$tmp_dir/vector_select_gt.s" \
  "$tmp_dir/vector_select_ge.s" \
  "$tmp_dir/vector_select_eq.s" \
  "$tmp_dir/vector_select_ne.s" \
  "$tmp_dir/vector_select_ult.s" \
  "$tmp_dir/vector_select_ule.s" \
  "$tmp_dir/vector_select_ugt.s" \
  "$tmp_dir/vector_select_uge.s" \
  "$tmp_dir/complex_vector_pipeline_harness.c" \
  -o "$tmp_dir/complex_vector_pipeline"

"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/complex_vector_pipeline"

"$zc_bin" "$source_root/examples/matrix_multiply.zc" \
  --emit-riscv-asm > "$tmp_dir/matrix_multiply.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/matrix_multiply.s" \
  "$source_root/test/qemu/matrix_multiply_harness.c" \
  -o "$tmp_dir/matrix_multiply"
"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/matrix_multiply"

"$zc_bin" "$source_root/examples/matrix_multiply_packed_b.zc" \
  --emit-riscv-asm > "$tmp_dir/matrix_multiply_packed_b.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/matrix_multiply_packed_b.s" \
  "$source_root/test/qemu/matrix_multiply_packed_b_harness.c" \
  -o "$tmp_dir/matrix_multiply_packed_b"
"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/matrix_multiply_packed_b"


"$zc_bin" "$source_root/examples/matrix_pack_b_then_multiply.zc" \
  --emit-riscv-asm > "$tmp_dir/matrix_pack_b_then_multiply.s"
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/matrix_pack_b_then_multiply.s" \
  "$source_root/test/qemu/matrix_pack_b_harness.c" \
  -o "$tmp_dir/matrix_pack_b_then_multiply"
"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/matrix_pack_b_then_multiply"

"$zc_bin" "$source_root/examples/vector_add_i16.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_add_i16.s"
"$zc_bin" "$source_root/examples/vector_add_i16_m2.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_add_i16_m2.s"
"$zc_bin" "$source_root/examples/vector_add_i16_m4.zc" \
  --emit-riscv-asm > "$tmp_dir/vector_add_i16_m4.s"
for example in vector_add_i8 vector_add_i64 vector_copy_i8 vector_copy_i64 \
  vector_select_i8_gt vector_select_i64_gt; do
  "$zc_bin" "$source_root/examples/${example}.zc" \
    --emit-riscv-asm > "$tmp_dir/${example}.s"
done
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/vector_add_i8.s" \
  "$tmp_dir/vector_add_i16.s" \
  "$tmp_dir/vector_add_i16_m2.s" \
  "$tmp_dir/vector_add_i16_m4.s" \
  "$tmp_dir/vector_add_i64.s" \
  "$tmp_dir/vector_copy_i8.s" \
  "$tmp_dir/vector_copy_i64.s" \
  "$tmp_dir/vector_select_i8_gt.s" \
  "$tmp_dir/vector_select_i64_gt.s" \
  "$source_root/test/qemu/vector_add_i16_harness.c" \
  -o "$tmp_dir/vector_add_i16"
"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/vector_add_i16"

for example in vector_strided_load vector_indexed_load vector_strided_store \
  vector_indexed_store vector_masked_strided_load vector_masked_indexed_load \
  vector_masked_strided_store vector_masked_indexed_store vector_mask_logical \
  vector_widen_add_i16_i32; do
  "$zc_bin" "$source_root/examples/${example}.zc" \
    --emit-riscv-asm > "$tmp_dir/${example}.s"
done
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d \
  "$tmp_dir/vector_strided_load.s" \
  "$tmp_dir/vector_indexed_load.s" \
  "$tmp_dir/vector_strided_store.s" \
  "$tmp_dir/vector_indexed_store.s" \
  "$tmp_dir/vector_masked_strided_load.s" \
  "$tmp_dir/vector_masked_indexed_load.s" \
  "$tmp_dir/vector_masked_strided_store.s" \
  "$tmp_dir/vector_masked_indexed_store.s" \
  "$tmp_dir/vector_mask_logical.s" \
  "$tmp_dir/vector_widen_add_i16_i32.s" \
  "$source_root/test/qemu/vector_memory_mask_widen_harness.c" \
  -o "$tmp_dir/vector_memory_mask_widen"
"$qemu_bin" -cpu "$qemu_cpu" "$tmp_dir/vector_memory_mask_widen"

echo "qemu-riscv64 validation passed"
