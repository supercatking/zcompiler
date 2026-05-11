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

"$zc_bin" "$source_root/examples/vector_mul.zc" --emit-mlir \
  > "$tmp_dir/vector_mul.mlir"
diff -u -B "$source_root/test/codegen/vector_mul.mlir" \
  "$tmp_dir/vector_mul.mlir"

"$zc_bin" "$source_root/examples/vector_mul.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_mul.riscv"
diff -u "$source_root/test/codegen/vector_mul.riscv" \
  "$tmp_dir/vector_mul.riscv"
for instruction in vsetvli vle32.v vmul.vv vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_mul.riscv"
done
if grep -q "vadd.vv" "$tmp_dir/vector_mul.riscv"; then
  echo "vector_mul unexpectedly contains vadd.vv" >&2
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

"$zc_bin" "$source_root/examples/vector_select_gt.zc" --emit-mlir \
  > "$tmp_dir/vector_select_gt.mlir"
diff -u -B "$source_root/test/codegen/vector_select_gt.mlir" \
  "$tmp_dir/vector_select_gt.mlir"

"$zc_bin" "$source_root/examples/vector_select_gt.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_select_gt.riscv"
diff -u "$source_root/test/codegen/vector_select_gt.riscv" \
  "$tmp_dir/vector_select_gt.riscv"
for instruction in vsetvli vle32.v vmslt.vv vmerge.vvm vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_select_gt.riscv"
done

"$zc_bin" "$source_root/examples/vector_select_eq.zc" --emit-mlir \
  > "$tmp_dir/vector_select_eq.mlir"
diff -u -B "$source_root/test/codegen/vector_select_eq.mlir" \
  "$tmp_dir/vector_select_eq.mlir"

"$zc_bin" "$source_root/examples/vector_select_eq.zc" --emit-riscv-asm \
  > "$tmp_dir/vector_select_eq.riscv"
diff -u "$source_root/test/codegen/vector_select_eq.riscv" \
  "$tmp_dir/vector_select_eq.riscv"
for instruction in vsetvli vle32.v vmseq.vv vmerge.vvm vse32.v; do
  grep -q "$instruction" "$tmp_dir/vector_select_eq.riscv"
done

for predicate in lt le ge ne; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-mlir \
    > "$tmp_dir/vector_select_${predicate}.mlir"
  diff -u -B "$source_root/test/codegen/vector_select_${predicate}.mlir" \
    "$tmp_dir/vector_select_${predicate}.mlir"

  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-riscv-asm \
    > "$tmp_dir/vector_select_${predicate}.riscv"
  diff -u "$source_root/test/codegen/vector_select_${predicate}.riscv" \
    "$tmp_dir/vector_select_${predicate}.riscv"
  for instruction in vsetvli vle32.v vmerge.vvm vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_select_${predicate}.riscv"
  done
done
grep -q vmslt.vv "$tmp_dir/vector_select_lt.riscv"
grep -q vmsle.vv "$tmp_dir/vector_select_le.riscv"
grep -q vmsle.vv "$tmp_dir/vector_select_ge.riscv"
grep -q vmsne.vv "$tmp_dir/vector_select_ne.riscv"

for predicate in ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-mlir \
    > "$tmp_dir/vector_select_${predicate}.mlir"
  diff -u -B "$source_root/test/codegen/vector_select_${predicate}.mlir" \
    "$tmp_dir/vector_select_${predicate}.mlir"

  "$zc_bin" "$source_root/examples/vector_select_${predicate}.zc" --emit-riscv-asm \
    > "$tmp_dir/vector_select_${predicate}.riscv"
  diff -u "$source_root/test/codegen/vector_select_${predicate}.riscv" \
    "$tmp_dir/vector_select_${predicate}.riscv"
  for instruction in vsetvli vle32.v vmerge.vvm vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_select_${predicate}.riscv"
  done
done
grep -q vmsltu.vv "$tmp_dir/vector_select_ult.riscv"
grep -q vmsleu.vv "$tmp_dir/vector_select_ule.riscv"
grep -q vmsltu.vv "$tmp_dir/vector_select_ugt.riscv"
grep -q vmsleu.vv "$tmp_dir/vector_select_uge.riscv"

for predicate in lt le gt ge eq ne ult ule ugt uge; do
  "$zc_bin" "$source_root/examples/vector_masked_add_${predicate}.zc" --emit-mlir \
    > "$tmp_dir/vector_masked_add_${predicate}.mlir"
  diff -u -B "$source_root/test/codegen/vector_masked_add_${predicate}.mlir" \
    "$tmp_dir/vector_masked_add_${predicate}.mlir"

  "$zc_bin" "$source_root/examples/vector_masked_add_${predicate}.zc" --emit-riscv-asm \
    > "$tmp_dir/vector_masked_add_${predicate}.riscv"
  diff -u "$source_root/test/codegen/vector_masked_add_${predicate}.riscv" \
    "$tmp_dir/vector_masked_add_${predicate}.riscv"
  for instruction in vsetvli vle32.v "vadd.vv" "v0.t" vmerge.vvm vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_masked_add_${predicate}.riscv"
  done
done
grep -q vmslt.vv "$tmp_dir/vector_masked_add_lt.riscv"
grep -q vmsle.vv "$tmp_dir/vector_masked_add_le.riscv"
grep -q vmslt.vv "$tmp_dir/vector_masked_add_gt.riscv"
grep -q vmsle.vv "$tmp_dir/vector_masked_add_ge.riscv"
grep -q vmseq.vv "$tmp_dir/vector_masked_add_eq.riscv"
grep -q vmsne.vv "$tmp_dir/vector_masked_add_ne.riscv"
grep -q vmsltu.vv "$tmp_dir/vector_masked_add_ult.riscv"
grep -q vmsleu.vv "$tmp_dir/vector_masked_add_ule.riscv"
grep -q vmsltu.vv "$tmp_dir/vector_masked_add_ugt.riscv"
grep -q vmsleu.vv "$tmp_dir/vector_masked_add_uge.riscv"

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

"$zc_bin" "$source_root/examples/print_i32.zc" --emit-riscv-asm \
  > "$tmp_dir/print_i32.riscv"
diff -u "$source_root/test/codegen/print_i32.riscv" "$tmp_dir/print_i32.riscv"
for instruction in "call zc_print_i32" "li a7, 64" ecall; do
  grep -q "$instruction" "$tmp_dir/print_i32.riscv"
done

"$zc_bin" "$source_root/examples/scalar_i32_wrap.zc" --emit-riscv-asm \
  > "$tmp_dir/scalar_i32_wrap.riscv"
diff -u "$source_root/test/codegen/scalar_i32_wrap.riscv" \
  "$tmp_dir/scalar_i32_wrap.riscv"
for instruction in addw subw mulw "call zc_print_i32"; do
  grep -q "$instruction" "$tmp_dir/scalar_i32_wrap.riscv"
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
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_mul.mlir" \
    -o "$tmp_dir/vector_mul.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_reduce_add.mlir" \
    -o "$tmp_dir/vector_reduce_add.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_select_gt.mlir" \
    -o "$tmp_dir/vector_select_gt.opt.mlir"
  /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_select_eq.mlir" \
    -o "$tmp_dir/vector_select_eq.opt.mlir"
  for predicate in lt le ge ne ult ule ugt uge; do
    /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_select_${predicate}.mlir" \
      -o "$tmp_dir/vector_select_${predicate}.opt.mlir"
  done
  for predicate in lt le gt ge eq ne ult ule ugt uge; do
    /home/zyz/mlir/build/bin/mlir-opt "$tmp_dir/vector_masked_add_${predicate}.mlir" \
      -o "$tmp_dir/vector_masked_add_${predicate}.opt.mlir"
  done
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
    "$tmp_dir/vector_mul.riscv" -o "$tmp_dir/vector_mul.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_mul.o" \
    > "$tmp_dir/vector_mul.objdump"
  for instruction in vsetvli vle32.v vmul.vv vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_mul.objdump"
  done
  if grep -q "vadd.vv" "$tmp_dir/vector_mul.objdump"; then
    echo "vector_mul objdump unexpectedly contains vadd.vv" >&2
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
    "$tmp_dir/vector_select_gt.riscv" -o "$tmp_dir/vector_select_gt.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_select_gt.o" \
    > "$tmp_dir/vector_select_gt.objdump"
  for instruction in vsetvli vle32.v vmslt.vv vmerge.vvm vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_select_gt.objdump"
  done
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/vector_select_eq.riscv" -o "$tmp_dir/vector_select_eq.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/vector_select_eq.o" \
    > "$tmp_dir/vector_select_eq.objdump"
  for instruction in vsetvli vle32.v vmseq.vv vmerge.vvm vse32.v; do
    grep -q "$instruction" "$tmp_dir/vector_select_eq.objdump"
  done
  for predicate in lt le ge ne; do
    riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
      "$tmp_dir/vector_select_${predicate}.riscv" \
      -o "$tmp_dir/vector_select_${predicate}.o"
    riscv64-linux-gnu-objdump -d "$tmp_dir/vector_select_${predicate}.o" \
      > "$tmp_dir/vector_select_${predicate}.objdump"
    for instruction in vsetvli vle32.v vmerge.vvm vse32.v; do
      grep -q "$instruction" "$tmp_dir/vector_select_${predicate}.objdump"
    done
  done
  grep -q vmslt.vv "$tmp_dir/vector_select_lt.objdump"
  grep -q vmsle.vv "$tmp_dir/vector_select_le.objdump"
  grep -q vmsle.vv "$tmp_dir/vector_select_ge.objdump"
  grep -q vmsne.vv "$tmp_dir/vector_select_ne.objdump"

  for predicate in ult ule ugt uge; do
    riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
      "$tmp_dir/vector_select_${predicate}.riscv" \
      -o "$tmp_dir/vector_select_${predicate}.o"
    riscv64-linux-gnu-objdump -d "$tmp_dir/vector_select_${predicate}.o" \
      > "$tmp_dir/vector_select_${predicate}.objdump"
    for instruction in vsetvli vle32.v vmerge.vvm vse32.v; do
      grep -q "$instruction" "$tmp_dir/vector_select_${predicate}.objdump"
    done
  done
  grep -q vmsltu.vv "$tmp_dir/vector_select_ult.objdump"
  grep -q vmsleu.vv "$tmp_dir/vector_select_ule.objdump"
  grep -q vmsltu.vv "$tmp_dir/vector_select_ugt.objdump"
  grep -q vmsleu.vv "$tmp_dir/vector_select_uge.objdump"
  for predicate in lt le gt ge eq ne ult ule ugt uge; do
    riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
      "$tmp_dir/vector_masked_add_${predicate}.riscv" \
      -o "$tmp_dir/vector_masked_add_${predicate}.o"
    riscv64-linux-gnu-objdump -d "$tmp_dir/vector_masked_add_${predicate}.o" \
      > "$tmp_dir/vector_masked_add_${predicate}.objdump"
    for instruction in vsetvli vle32.v "vadd.vv" "v0.t" \
      vmerge.vvm vse32.v; do
      grep -q "$instruction" "$tmp_dir/vector_masked_add_${predicate}.objdump"
    done
  done
  grep -q vmslt.vv "$tmp_dir/vector_masked_add_lt.objdump"
  grep -q vmsle.vv "$tmp_dir/vector_masked_add_le.objdump"
  grep -q vmslt.vv "$tmp_dir/vector_masked_add_gt.objdump"
  grep -q vmsle.vv "$tmp_dir/vector_masked_add_ge.objdump"
  grep -q vmseq.vv "$tmp_dir/vector_masked_add_eq.objdump"
  grep -q vmsne.vv "$tmp_dir/vector_masked_add_ne.objdump"
  grep -q vmsltu.vv "$tmp_dir/vector_masked_add_ult.objdump"
  grep -q vmsleu.vv "$tmp_dir/vector_masked_add_ule.objdump"
  grep -q vmsltu.vv "$tmp_dir/vector_masked_add_ugt.objdump"
  grep -q vmsleu.vv "$tmp_dir/vector_masked_add_uge.objdump"
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/complex_vector_pipeline.riscv" \
    -o "$tmp_dir/complex_vector_pipeline.o"
  riscv64-linux-gnu-objdump -d "$tmp_dir/complex_vector_pipeline.o" \
    > "$tmp_dir/complex_vector_pipeline.objdump"
  for instruction in vsetvli vle32.v vadd.vv vmul.vx vse32.v vmv.s.x \
    vredsum.vs vmv.x.s; do
    grep -q "$instruction" "$tmp_dir/complex_vector_pipeline.objdump"
  done
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/print_i32.riscv" -o "$tmp_dir/print_i32.o"
  riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
    "$tmp_dir/scalar_i32_wrap.riscv" -o "$tmp_dir/scalar_i32_wrap.o"
fi
