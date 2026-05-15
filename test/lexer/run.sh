#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"$zc_bin" "$source_root/examples/hello.zc" --emit-tokens \
  > "$tmp_dir/hello.tokens"
diff -u "$source_root/test/lexer/hello.tokens" "$tmp_dir/hello.tokens"

"$zc_bin" "$source_root/examples/calls.zc" --emit-tokens \
  > "$tmp_dir/calls.tokens"
diff -u "$source_root/test/lexer/calls.tokens" "$tmp_dir/calls.tokens"

"$zc_bin" "$source_root/examples/arrays.zc" --emit-tokens \
  > "$tmp_dir/arrays.tokens"
diff -u "$source_root/test/lexer/arrays.tokens" "$tmp_dir/arrays.tokens"

"$zc_bin" "$source_root/examples/matrix_multiply.zc" --emit-tokens \
  > "$tmp_dir/matrix_multiply.tokens"
diff -u "$source_root/test/lexer/matrix_multiply.tokens" \
  "$tmp_dir/matrix_multiply.tokens"

"$zc_bin" "$source_root/examples/matrix_multiply_packed_b.zc" --emit-tokens \
  > "$tmp_dir/matrix_multiply_packed_b.tokens"
diff -u "$source_root/test/lexer/matrix_multiply_packed_b.tokens" \
  "$tmp_dir/matrix_multiply_packed_b.tokens"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-tokens \
  > "$tmp_dir/vector_add.tokens"
diff -u "$source_root/test/lexer/vector_add.tokens" \
  "$tmp_dir/vector_add.tokens"

"$zc_bin" "$source_root/examples/vector_copy.zc" --emit-tokens \
  > "$tmp_dir/vector_copy.tokens"
diff -u "$source_root/test/lexer/vector_copy.tokens" \
  "$tmp_dir/vector_copy.tokens"

"$zc_bin" "$source_root/examples/vector_scale.zc" --emit-tokens \
  > "$tmp_dir/vector_scale.tokens"
diff -u "$source_root/test/lexer/vector_scale.tokens" \
  "$tmp_dir/vector_scale.tokens"

"$zc_bin" "$source_root/examples/vector_mul.zc" --emit-tokens \
  > "$tmp_dir/vector_mul.tokens"
diff -u "$source_root/test/lexer/vector_mul.tokens" \
  "$tmp_dir/vector_mul.tokens"

"$zc_bin" "$source_root/examples/vector_reduce_add.zc" --emit-tokens \
  > "$tmp_dir/vector_reduce_add.tokens"
diff -u "$source_root/test/lexer/vector_reduce_add.tokens" \
  "$tmp_dir/vector_reduce_add.tokens"

"$zc_bin" "$source_root/examples/vector_select_gt.zc" --emit-tokens \
  > "$tmp_dir/vector_select_gt.tokens"
diff -u "$source_root/test/lexer/vector_select_gt.tokens" \
  "$tmp_dir/vector_select_gt.tokens"

"$zc_bin" "$source_root/examples/vector_select_eq.zc" --emit-tokens \
  > "$tmp_dir/vector_select_eq.tokens"
diff -u "$source_root/test/lexer/vector_select_eq.tokens" \
  "$tmp_dir/vector_select_eq.tokens"

for predicate in lt le ge ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-tokens \
    > "$tmp_dir/vector_select_${predicate}.tokens"
  diff -u "$source_root/test/lexer/vector_select_${predicate}.tokens" \
    "$tmp_dir/vector_select_${predicate}.tokens"
done

for predicate in lt le gt ge eq ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_masked_add_${predicate}.zc" --emit-tokens \
    > "$tmp_dir/vector_masked_add_${predicate}.tokens"
  diff -u "$source_root/test/lexer/vector_masked_add_${predicate}.tokens" \
    "$tmp_dir/vector_masked_add_${predicate}.tokens"
done

for op in sub mul; do
  "$zc_bin" "$source_root/examples/vector_masked_${op}_gt.zc" --emit-tokens \
    > "$tmp_dir/vector_masked_${op}_gt.tokens"
  diff -u "$source_root/test/lexer/vector_masked_${op}_gt.tokens" \
    "$tmp_dir/vector_masked_${op}_gt.tokens"
done

"$zc_bin" "$source_root/examples/vector_masked_store_gt.zc" --emit-tokens \
  > "$tmp_dir/vector_masked_store_gt.tokens"
diff -u "$source_root/test/lexer/vector_masked_store_gt.tokens" \
  "$tmp_dir/vector_masked_store_gt.tokens"

"$zc_bin" "$source_root/examples/vector_masked_load_gt.zc" --emit-tokens \
  > "$tmp_dir/vector_masked_load_gt.tokens"
diff -u "$source_root/test/lexer/vector_masked_load_gt.tokens" \
  "$tmp_dir/vector_masked_load_gt.tokens"


for example in matrix_pack_b_then_multiply vector_add_i16 vector_add_i16_m2   vector_add_i16_m4 vector_strided_load vector_indexed_load vector_strided_store   vector_indexed_store vector_mask_logical vector_widen_add_i16_i32; do
  "$zc_bin" "$source_root/examples/${example}.zc" --emit-tokens     > "$tmp_dir/${example}.tokens"
  diff -u "$source_root/test/lexer/${example}.tokens"     "$tmp_dir/${example}.tokens"
done

"$zc_bin" "$source_root/examples/print_i32.zc" --emit-tokens \
  > "$tmp_dir/print_i32.tokens"
diff -u "$source_root/test/lexer/print_i32.tokens" \
  "$tmp_dir/print_i32.tokens"

set +e
"$zc_bin" "$source_root/test/lexer/invalid.zc" --emit-tokens \
  > "$tmp_dir/invalid.out" 2> "$tmp_dir/invalid.err"
status="$?"
set -e

if [ "$status" -eq 0 ]; then
  echo "expected invalid lexer input to fail" >&2
  exit 1
fi

diff -u "$source_root/test/lexer/invalid.err" "$tmp_dir/invalid.err"
