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

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-ast \
  > "$tmp_dir/vector_add.ast"
diff -u "$source_root/test/parser/vector_add.ast" \
  "$tmp_dir/vector_add.ast"

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
