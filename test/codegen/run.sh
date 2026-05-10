#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"$zc_bin" "$source_root/examples/hello.zc" --emit-mlir \
  > "$tmp_dir/hello.mlir"
diff -u "$source_root/test/codegen/hello.mlir" "$tmp_dir/hello.mlir"

"$zc_bin" "$source_root/examples/hello.zc" --emit-zc-mlir \
  > "$tmp_dir/hello.zc.mlir"
diff -u "$source_root/test/codegen/hello.zc.mlir" "$tmp_dir/hello.zc.mlir"

"$zc_bin" "$source_root/examples/hello.zc" --emit-lowered-mlir \
  > "$tmp_dir/hello.lowered.mlir"
diff -u "$source_root/test/codegen/hello.lowered.mlir" \
  "$tmp_dir/hello.lowered.mlir"

if grep -q 'zc\.' "$tmp_dir/hello.lowered.mlir"; then
  echo "lowered MLIR still contains zc dialect operations" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/hello.zc" --emit-llvm \
  > "$tmp_dir/hello.ll"
diff -u "$source_root/test/codegen/hello.ll" "$tmp_dir/hello.ll"

if [ -x /home/zyz/mlir/build/bin/mlir-opt ]; then
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/hello.mlir" \
    -o "$tmp_dir/hello.opt.mlir"
fi

if [ -x /home/zyz/mlir/build/bin/llvm-as ]; then
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/hello.ll" \
    -o "$tmp_dir/hello.bc"
fi

