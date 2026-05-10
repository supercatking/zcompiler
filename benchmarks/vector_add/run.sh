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

count_instruction() {
  local instruction="$1"
  local file="$2"
  grep -c "$instruction" "$file" || true
}

reference_obj_size="$(stat -c%s "$out_dir/reference_rvv.o")"
zcompiler_obj_size="$(stat -c%s "$out_dir/zcompiler_vector_add.o")"

cat > "$out_dir/result.json" <<EOF
{
  "schema_version": 1,
  "benchmark_id": "vector_add_rvv_artifacts",
  "source_program": "examples/vector_add.zc",
  "reference_source": "benchmarks/vector_add/reference_rvv.c",
  "output_directory": "build/benchmarks/vector_add",
  "tools": {
    "reference_compiler": "riscv64-linux-gnu-gcc",
    "zcompiler": "build/tools/zc/zc",
    "assembler": "riscv64-linux-gnu-as",
    "objdump": "riscv64-linux-gnu-objdump"
  },
  "required_instructions": ["vsetvli", "vle32.v", "vadd.vv", "vse32.v"],
  "artifacts": {
    "reference": {
      "assembly": "reference_rvv.s",
      "object": "reference_rvv.o",
      "objdump": "reference_rvv.objdump",
      "object_size_bytes": ${reference_obj_size}
    },
    "zcompiler": {
      "assembly": "zcompiler_vector_add.s",
      "object": "zcompiler_vector_add.o",
      "objdump": "zcompiler_vector_add.objdump",
      "object_size_bytes": ${zcompiler_obj_size}
    }
  },
  "instruction_counts": {
    "reference": {
      "vsetvli": $(count_instruction vsetvli "$out_dir/reference_rvv.objdump"),
      "vle32.v": $(count_instruction vle32.v "$out_dir/reference_rvv.objdump"),
      "vadd.vv": $(count_instruction vadd.vv "$out_dir/reference_rvv.objdump"),
      "vse32.v": $(count_instruction vse32.v "$out_dir/reference_rvv.objdump")
    },
    "zcompiler": {
      "vsetvli": $(count_instruction vsetvli "$out_dir/zcompiler_vector_add.objdump"),
      "vle32.v": $(count_instruction vle32.v "$out_dir/zcompiler_vector_add.objdump"),
      "vadd.vv": $(count_instruction vadd.vv "$out_dir/zcompiler_vector_add.objdump"),
      "vse32.v": $(count_instruction vse32.v "$out_dir/zcompiler_vector_add.objdump")
    }
  },
  "status": "passed"
}
EOF

cat > "$out_dir/result.md" <<EOF
# Vector Add RVV Artifact Result

Status: passed

Source program: \`examples/vector_add.zc\`

Reference source: \`benchmarks/vector_add/reference_rvv.c\`

Output directory: \`build/benchmarks/vector_add\`

## Object Sizes

- Reference RVV object: ${reference_obj_size} bytes
- zcompiler RVV object: ${zcompiler_obj_size} bytes

## Required Instructions

| Instruction | Reference Count | zcompiler Count |
| --- | ---: | ---: |
| vsetvli | $(count_instruction vsetvli "$out_dir/reference_rvv.objdump") | $(count_instruction vsetvli "$out_dir/zcompiler_vector_add.objdump") |
| vle32.v | $(count_instruction vle32.v "$out_dir/reference_rvv.objdump") | $(count_instruction vle32.v "$out_dir/zcompiler_vector_add.objdump") |
| vadd.vv | $(count_instruction vadd.vv "$out_dir/reference_rvv.objdump") | $(count_instruction vadd.vv "$out_dir/zcompiler_vector_add.objdump") |
| vse32.v | $(count_instruction vse32.v "$out_dir/reference_rvv.objdump") | $(count_instruction vse32.v "$out_dir/zcompiler_vector_add.objdump") |

## Reproduce

\`\`\`bash
./benchmarks/vector_add/run.sh
\`\`\`
EOF

echo "Vector add artifacts written to $out_dir"
