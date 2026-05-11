#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"

python3 "$source_root/test/correctness/vector_add_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_copy_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_scale_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_mul_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_reduce_add_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_select_gt_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_select_eq_host.py" \
  "$zc_bin" \
  "$source_root"

for predicate in lt le ge ne ult ule ugt uge; do
  python3 "$source_root/test/correctness/vector_select_${predicate}_host.py" \
    "$zc_bin" \
    "$source_root"
done

python3 "$source_root/test/correctness/vector_masked_add_gt_host.py" \
  "$zc_bin" \
  "$source_root"

python3 "$source_root/test/correctness/vector_masked_add_predicates_host.py" \
  "$zc_bin" \
  "$source_root"
