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

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-tokens \
  > "$tmp_dir/vector_add.tokens"
diff -u "$source_root/test/lexer/vector_add.tokens" \
  "$tmp_dir/vector_add.tokens"

"$zc_bin" "$source_root/examples/vector_copy.zc" --emit-tokens \
  > "$tmp_dir/vector_copy.tokens"
diff -u "$source_root/test/lexer/vector_copy.tokens" \
  "$tmp_dir/vector_copy.tokens"

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
