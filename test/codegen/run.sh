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

"$zc_bin" "$source_root/examples/arrays.zc" --emit-mlir \
  > "$tmp_dir/arrays.mlir"
diff -u -B "$source_root/test/codegen/arrays.mlir" "$tmp_dir/arrays.mlir"

"$zc_bin" "$source_root/examples/arrays.zc" --emit-llvm \
  > "$tmp_dir/arrays.ll"
diff -u "$source_root/test/codegen/arrays.ll" "$tmp_dir/arrays.ll"

"$zc_bin" "$source_root/examples/arrays.zc" --emit-riscv-asm \
  > "$tmp_dir/arrays.riscv"
diff -u "$source_root/test/codegen/arrays.riscv" "$tmp_dir/arrays.riscv"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-mlir \
  > "$tmp_dir/vector_add.mlir"
diff -u -B "$source_root/test/codegen/vector_add.mlir" \
  "$tmp_dir/vector_add.mlir"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_add.riscv"
diff -u "$source_root/test/codegen/vector_add.riscv" \
  "$tmp_dir/vector_add.riscv"
for instruction in vsetvli vle32.v vadd.vv vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_add.riscv"
done

"$zc_bin" "$source_root/examples/vector_copy.zc" --emit-mlir \
  > "$tmp_dir/vector_copy.mlir"
diff -u -B "$source_root/test/codegen/vector_copy.mlir" \
  "$tmp_dir/vector_copy.mlir"

"$zc_bin" "$source_root/examples/vector_copy.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_copy.riscv"
diff -u "$source_root/test/codegen/vector_copy.riscv" \
  "$tmp_dir/vector_copy.riscv"
for instruction in vsetvli vle32.v vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_copy.riscv"
done
if grep -q "vadd.vv" "$tmp_dir/vector_copy.riscv"; then
  echo "vector_copy unexpectedly contains vadd.vv" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/vector_scale.zc" --emit-mlir \
  > "$tmp_dir/vector_scale.mlir"
diff -u -B "$source_root/test/codegen/vector_scale.mlir" \
  "$tmp_dir/vector_scale.mlir"

"$zc_bin" "$source_root/examples/vector_scale.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_scale.riscv"
diff -u "$source_root/test/codegen/vector_scale.riscv" \
  "$tmp_dir/vector_scale.riscv"
for instruction in vsetvli vle32.v vmul.vx vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_scale.riscv"
done
if grep -q "vadd.vv" "$tmp_dir/vector_scale.riscv"; then
  echo "vector_scale unexpectedly contains vadd.vv" >&2
  exit 1
fi

"$zc_bin" "$source_root/examples/vector_reduce_add.zc" --emit-mlir \
  > "$tmp_dir/vector_reduce_add.mlir"
diff -u -B "$source_root/test/codegen/vector_reduce_add.mlir" \
  "$tmp_dir/vector_reduce_add.mlir"

"$zc_bin" "$source_root/examples/vector_reduce_add.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_reduce_add.riscv"
diff -u "$source_root/test/codegen/vector_reduce_add.riscv" \
  "$tmp_dir/vector_reduce_add.riscv"
for instruction in vsetvli vle32.v vmv.s.x vredsum.vs vmv.x.s; do
  grep -q "$instruction" "$tmp_dir/vector_reduce_add.riscv"
done

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" --emit-ast \
  > "$tmp_dir/complex_vector_pipeline.ast"
for node in VectorAddStmt VectorScaleStmt VectorReduceAddStmt VectorCopyStmt; do
  grep -q "$node" "$tmp_dir/complex_vector_pipeline.ast"
done

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" --emit-mlir \
  > "$tmp_dir/complex_vector_pipeline.mlir"
for op in vector.transfer_read arith.addi vector.broadcast arith.muli \
  vector.reduction vector.transfer_write; do
  grep -q "$op" "$tmp_dir/complex_vector_pipeline.mlir"
done

"$zc_bin" "$source_root/examples/complex_vector_pipeline.zc" --emit-riscv-asm \
  > "$tmp_dir/complex_vector_pipeline.riscv"
for instruction in vsetvli vle32.v vadd.vv vmul.vx vse32.v vmv.s.x \
  vredsum.vs vmv.x.s; do
  grep -q "$instruction" "$tmp_dir/complex_vector_pipeline.riscv"
done

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
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/arrays.mlir" \
    -o "$tmp_dir/arrays.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_add.mlir" \
    -o "$tmp_dir/vector_add.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_copy.mlir" \
    -o "$tmp_dir/vector_copy.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_scale.mlir" \
    -o "$tmp_dir/vector_scale.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_reduce_add.mlir" \
    -o "$tmp_dir/vector_reduce_add.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/complex_vector_pipeline.mlir" \
    -o "$tmp_dir/complex_vector_pipeline.opt.mlir"
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
  /home/zyz/mlir/build/bin/llvm-as "$tmp_dir/arrays.ll" \
    -o "$tmp_dir/arrays.bc"
fi

if command -v riscv64-linux-gnu-as >/dev/null; then
  riscv64-linux-gnu-as "$tmp_dir/hello.riscv" -o "$tmp_dir/hello.o"
  riscv64-linux-gnu-as "$tmp_dir/calls.riscv" -o "$tmp_dir/calls.o"
  riscv64-linux-gnu-as "$tmp_dir/arrays.riscv" -o "$tmp_dir/arrays.o"
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/vector_add.riscv" -o "$tmp_dir/vector_add.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_add.o" \
    > "$tmp_dir/vector_add.objdump"
  for instruction in vsetvli vle32.v vadd.vv vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_add.objdump"
  done
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/vector_copy.riscv" -o "$tmp_dir/vector_copy.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_copy.o" \
    > "$tmp_dir/vector_copy.objdump"
  for instruction in vsetvli vle32.v vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_copy.objdump"
  done
  if grep -q "vadd.vv" "$tmp_dir/vector_copy.objdump"; then
    echo "vector_copy objdump unexpectedly contains vadd.vv" >&2
    exit 1
  fi
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/vector_scale.riscv" -o "$tmp_dir/vector_scale.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_scale.o" \
    > "$tmp_dir/vector_scale.objdump"
  for instruction in vsetvli vle32.v vmul.vx vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_scale.objdump"
  done
  if grep -q "vadd.vv" "$tmp_dir/vector_scale.objdump"; then
    echo "vector_scale objdump unexpectedly contains vadd.vv" >&2
    exit 1
  fi
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/vector_reduce_add.riscv" -o "$tmp_dir/vector_reduce_add.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_reduce_add.o" \
    > "$tmp_dir/vector_reduce_add.objdump"
  for instruction in vsetvli vle32.v vmv.s.x vredsum.vs vmv.x.s; do
    grep -q "$instruction" "$tmp_dir/vector_reduce_add.objdump"
  done
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/complex_vector_pipeline.riscv" \
    -o "$tmp_dir/complex_vector_pipeline.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/complex_vector_pipeline.o" \
    > "$tmp_dir/complex_vector_pipeline.objdump"
  for instruction in vsetvli vle32.v vadd.vv vmul.vx vse32.v vmv.s.x \
    vredsum.vs vmv.x.s; do
    grep -q "$instruction" "$tmp_dir/complex_vector_pipeline.objdump"
  done
fi
