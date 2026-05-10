#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

"$zc_bin" "$source_root/examples/hello.zc" --emit-mlir \
  > "$tmp_dir/hello.mlir"
diff -u -B "$source_root/test/codegen/hello.mlir" "$tmp_dir/hello.mlir"

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

"$zc_bin" "$source_root/examples/hello.zc" --emit-riscv-asm \
  > "$tmp_dir/hello.riscv"
diff -u "$source_root/test/codegen/hello.riscv" "$tmp_dir/hello.riscv"

"$zc_bin" "$source_root/examples/calls.zc" --emit-mlir \
  > "$tmp_dir/calls.mlir"
diff -u -B "$source_root/test/codegen/calls.mlir" "$tmp_dir/calls.mlir"

"$zc_bin" "$source_root/examples/calls.zc" --emit-llvm \
  > "$tmp_dir/calls.ll"
diff -u "$source_root/test/codegen/calls.ll" "$tmp_dir/calls.ll"

"$zc_bin" "$source_root/examples/calls.zc" --emit-riscv-asm \
  > "$tmp_dir/calls.riscv"
diff -u "$source_root/test/codegen/calls.riscv" "$tmp_dir/calls.riscv"

"$zc_bin" "$source_root/examples/control.zc" --emit-llvm \
  > "$tmp_dir/control.ll"
diff -u "$source_root/test/codegen/control.ll" "$tmp_dir/control.ll"

"$zc_bin" "$source_root/examples/while.zc" --emit-llvm \
  > "$tmp_dir/while.ll"
diff -u "$source_root/test/codegen/while.ll" "$tmp_dir/while.ll"

if [ -x /home/zyz/mlir/build/bin/mlir-opt ]; then
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/hello.mlir" \
    -o "$tmp_dir/hello.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/calls.mlir" \
    -o "$tmp_dir/calls.opt.mlir"
fi

if [ -x /home/zyz/mlir/build/bin/llvm-as ]; then
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/hello.ll" \
    -o "$tmp_dir/hello.bc"
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/control.ll" \
    -o "$tmp_dir/control.bc"
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/while.ll" \
    -o "$tmp_dir/while.bc"
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/calls.ll" \
    -o "$tmp_dir/calls.bc"
fi

if command -v riscv64-linux-gnu-as >/dev/null; then
  riscv64-linux-gnu-as "$tmp_dir/hello.riscv" -o "$tmp_dir/hello.o"
  riscv64-linux-gnu-as "$tmp_dir/calls.riscv" -o "$tmp_dir/calls.o"
fi
