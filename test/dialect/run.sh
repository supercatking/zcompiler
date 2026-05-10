#!/usr/bin/env bash
set -euo pipefail

zc_opt_bin="$1"
source_root="$2"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"$zc_opt_bin" "$source_root/test/dialect/registered.mlir" \
  -o "$tmp_dir/registered.checked.mlir"

grep -q 'zc.constant' "$tmp_dir/registered.checked.mlir"
grep -q 'zc.add' "$tmp_dir/registered.checked.mlir"

"$zc_opt_bin" "$source_root/test/dialect/registered.mlir" \
  --lower-zc-to-standard \
  -o "$tmp_dir/lowered.mlir"

if grep -q 'zc\.' "$tmp_dir/lowered.mlir"; then
  echo "lower-zc-to-standard left zc operations in output" >&2
  exit 1
fi

diff -u -B "$source_root/test/dialect/lowered.mlir" "$tmp_dir/lowered.mlir"
