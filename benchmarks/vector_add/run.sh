#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source_root="$(cd "$script_dir/../.." && pwd)"
out_dir="$source_root/build/benchmarks/vector_add"
mkdir -p "$out_dir"

zc_bin="$source_root/build/tools/zc/zc"
if [ ! -x "$zc_bin" ]; then
  cmake --build "$source_root/build"
fi

riscv64-linux-gnu-gcc -S "$script_dir/reference_rvv.c" \
  -o "$out_dir/reference_rvv.s" \
  -O2 -march=rv64gcv -mabi=lp64d

riscv64-linux-gnu-gcc -c "$script_dir/reference_rvv.c" \
  -o "$out_dir/reference_rvv.o" \
  -O2 -march=rv64gcv -mabi=lp64d

riscv64-linux-gnu-objdump -d "$out_dir/reference_rvv.o" \
  > "$out_dir/reference_rvv.objdump"

"$zc_bin" "$source_root/examples/vector_add.zc" --emit-riscv-asm \
  > "$out_dir/zcompiler_vector_add.s"

riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
  "$out_dir/zcompiler_vector_add.s" \
  -o "$out_dir/zcompiler_vector_add.o"

riscv64-linux-gnu-objdump -d "$out_dir/zcompiler_vector_add.o" \
  > "$out_dir/zcompiler_vector_add.objdump"

for instruction in vsetvli vle32.v vadd.vv vse32.v; do
  grep -q "$instruction" "$out_dir/reference_rvv.objdump"
  grep -q "$instruction" "$out_dir/zcompiler_vector_add.objdump"
done

echo "Vector add artifacts written to $out_dir"
