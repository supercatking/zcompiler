#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"$zc_bin" "$source_root/examples/hello.zc" --emit-ast \
  > "$tmp_dir/hello.ast"
diff -u "$source_root/test/parser/hello.ast" "$tmp_dir/hello.ast"

"$zc_bin" "$source_root/examples/control.zc" --emit-ast \
  > "$tmp_dir/control.ast"
diff -u "$source_root/test/parser/control.ast" "$tmp_dir/control.ast"

"$zc_bin" "$source_root/examples/while.zc" --emit-ast \
  > "$tmp_dir/while.ast"
diff -u "$source_root/test/parser/while.ast" "$tmp_dir/while.ast"

"$zc_bin" "$source_root/examples/calls.zc" --emit-ast \
  > "$tmp_dir/calls.ast"
diff -u "$source_root/test/parser/calls.ast" "$tmp_dir/calls.ast"

"$zc_bin" "$source_root/examples/arrays.zc" --emit-ast \
  > "$tmp_dir/arrays.ast"
diff -u "$source_root/test/parser/arrays.ast" "$tmp_dir/arrays.ast"

"$zc_bin" "$source_root/examples/matrix_multiply.zc" --emit-ast \
  > "$tmp_dir/matrix_multiply.ast"
diff -u "$source_root/test/parser/matrix_multiply.ast" \
  "$tmp_dir/matrix_multiply.ast"

"$zc_bin" "$source_root/examples/matrix_multiply_packed_b.zc" --emit-ast \
  > "$tmp_dir/matrix_multiply_packed_b.ast"
diff -u "$source_root/test/parser/matrix_multiply_packed_b.ast" \
  "$tmp_dir/matrix_multiply_packed_b.ast"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-ast \
  > "$tmp_dir/vector_add.ast"
diff -u "$source_root/test/parser/vector_add.ast" \
  "$tmp_dir/vector_add.ast"

"$zc_bin" "$source_root/examples/vector_copy.zc" --emit-ast \
  > "$tmp_dir/vector_copy.ast"
diff -u "$source_root/test/parser/vector_copy.ast" \
  "$tmp_dir/vector_copy.ast"

"$zc_bin" "$source_root/examples/vector_scale.zc" --emit-ast \
  > "$tmp_dir/vector_scale.ast"
diff -u "$source_root/test/parser/vector_scale.ast" \
  "$tmp_dir/vector_scale.ast"

"$zc_bin" "$source_root/examples/vector_mul.zc" --emit-ast \
  > "$tmp_dir/vector_mul.ast"
diff -u "$source_root/test/parser/vector_mul.ast" \
  "$tmp_dir/vector_mul.ast"

"$zc_bin" "$source_root/examples/vector_reduce_add.zc" --emit-ast \
  > "$tmp_dir/vector_reduce_add.ast"
diff -u "$source_root/test/parser/vector_reduce_add.ast" \
  "$tmp_dir/vector_reduce_add.ast"

"$zc_bin" "$source_root/examples/vector_select_gt.zc" --emit-ast \
  > "$tmp_dir/vector_select_gt.ast"
diff -u "$source_root/test/parser/vector_select_gt.ast" \
  "$tmp_dir/vector_select_gt.ast"

"$zc_bin" "$source_root/examples/vector_select_eq.zc" --emit-ast \
  > "$tmp_dir/vector_select_eq.ast"
diff -u "$source_root/test/parser/vector_select_eq.ast" \
  "$tmp_dir/vector_select_eq.ast"

for predicate in lt le ge ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-ast \
    > "$tmp_dir/vector_select_${predicate}.ast"
  diff -u "$source_root/test/parser/vector_select_${predicate}.ast" \
    "$tmp_dir/vector_select_${predicate}.ast"
done

for predicate in lt le gt ge eq ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_masked_add_${predicate}.zc" --emit-ast \
    > "$tmp_dir/vector_masked_add_${predicate}.ast"
  diff -u "$source_root/test/parser/vector_masked_add_${predicate}.ast" \
    "$tmp_dir/vector_masked_add_${predicate}.ast"
done

for op in sub mul; do
  "$zc_bin" "$source_root/examples/vector_masked_${op}_gt.zc" --emit-ast \
    > "$tmp_dir/vector_masked_${op}_gt.ast"
  diff -u "$source_root/test/parser/vector_masked_${op}_gt.ast" \
    "$tmp_dir/vector_masked_${op}_gt.ast"
done

"$zc_bin" "$source_root/examples/vector_masked_store_gt.zc" --emit-ast \
  > "$tmp_dir/vector_masked_store_gt.ast"
diff -u "$source_root/test/parser/vector_masked_store_gt.ast" \
  "$tmp_dir/vector_masked_store_gt.ast"

"$zc_bin" "$source_root/examples/vector_masked_load_gt.zc" --emit-ast \
  > "$tmp_dir/vector_masked_load_gt.ast"
diff -u "$source_root/test/parser/vector_masked_load_gt.ast" \
  "$tmp_dir/vector_masked_load_gt.ast"


for example in matrix_pack_b_then_multiply vector_add_i8 vector_add_i16   vector_add_i16_m2 vector_add_i16_m4 vector_add_i64 vector_copy_i8   vector_copy_i64 vector_mul_i8 vector_mul_i64 vector_scale_i8 vector_scale_i64   vector_select_i8_gt vector_select_i64_gt vector_strided_load vector_indexed_load   vector_strided_store vector_indexed_store vector_masked_strided_load   vector_masked_indexed_load vector_masked_strided_store   vector_masked_indexed_store vector_mask_logical vector_widen_add_i16_i32; do
  "$zc_bin" "$source_root/examples/${example}.zc" --emit-ast     > "$tmp_dir/${example}.ast"
  diff -u "$source_root/test/parser/${example}.ast"     "$tmp_dir/${example}.ast"
done

"$zc_bin" "$source_root/examples/print_i32.zc" --emit-ast \
  > "$tmp_dir/print_i32.ast"
diff -u "$source_root/test/parser/print_i32.ast" "$tmp_dir/print_i32.ast"

set +e
"$zc_bin" "$source_root/test/parser/invalid.zc" --emit-ast \
  > "$tmp_dir/invalid.out" 2> "$tmp_dir/invalid.err"
status="$?"
set -e

if [ "$status" -eq 0 ]; then
  echo "expected invalid parser input to fail" >&2
  exit 1
fi

diff -u "$source_root/test/parser/invalid.err" "$tmp_dir/invalid.err"
