# Current Compiler Capabilities

This note records what the current toy compiler can compile and where the
current boundaries are.

## 当前能编译的程序形态

目前 `zc` 前端能处理以下源语言特性：

- `func` 函数定义，支持 `i32` 和 `ptr<i32>` 参数。
- `let` 局部绑定、直线赋值、`return`。
- 整数表达式：`+`、`-`、`*`、`/`。
- 比较表达式：`<`、`<=`、`>`、`>=`、`==`、`!=`。
- 函数调用。
- 标量内存访问：`load a[i]` 和 `store c[i] = value;`。
- 控制流：`if/else`、`while`。
- 终端输出：`print_i32 expr;`，在 RISC-V Linux/QEMU 路径输出十进制
  `i32` 值并换行。
- 目标无关向量 kernel：
  - `vector_add c, a, b, n;`
  - `vector_copy c, a, n;`
  - `vector_scale c, a, factor, n;`
  - `vector_mul c, a, b, n;`
  - `vector_reduce_add sum, a, n;`
  - `vector_select_gt out, lhs, rhs, true_values, false_values, n;`
  - `vector_select_eq out, lhs, rhs, true_values, false_values, n;`

## 当前输出能力

当前编译器可以生成：

- token stream: `--emit-tokens`
- AST dump: `--emit-ast`
- MLIR: `--emit-mlir`
- LLVM IR: `--emit-llvm`
- RISC-V assembly: `--emit-riscv-asm`

`print_i32` 当前是 RISC-V runtime 功能，会生成一个内建
`zc_print_i32` helper，并通过 RISC-V Linux `write` syscall 输出到 stdout。

向量 kernel 的 MLIR 路径会生成 `scf`、`vector`、`arith`、`func`
dialect 组合，使用 `vector.create_mask` 处理尾部长度，因此 `n` 不要求是
4 的整数倍。

向量 kernel 的 RISC-V 路径目前是直接 RVV reference backend，会生成
`vsetvli`、`vle32.v`、`vse32.v`、`vadd.vv`、`vmul.vx`、`vredsum.vs`
等 RVV 指令，并可以用 `riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d`
汇编成 `.o` 文件。
Vector multiply currently maps to vmul.vv in the direct RVV reference backend.
Vector compare/select currently covers signed lt/le/gt/ge/eq/ne plus unsigned ult/ule/ugt/uge and maps to RVV signed/unsigned compare masks plus vmerge.vvm.

在当前 WSL 环境中，`/home/qemu/qemu/build-riscv64-user/qemu-riscv64`
已经可以运行 zcompiler 生成并链接出的 RISC-V64 Linux ELF。CTest 中的
`qemu-riscv64` target 会验证 `print_i32` stdout 和 RVV pipeline 运行结果。

当前 RVV 状态是 RVV 1.0 compatible subset，而不是 full RVV 1.0 coverage。
具体合规矩阵见 [rvv-1.0-compliance.md](rvv-1.0-compliance.md)。

## 当前最复杂稳定示例

示例源码：

```text
examples/complex_vector_pipeline.zc
```

它覆盖两个函数：

- `complex_vector_pipeline`
  - `tmp = a + b`
  - `out = tmp * factor`
  - `sum = reduce_add(out)`
  - 返回 `sum`
- `copy_then_sum`
  - `out = a`
  - `sum = reduce_add(out)`
  - 返回 `sum`

这个例子同时覆盖：

- 多函数 module。
- 多个 `ptr<i32>` 参数。
- 多个向量 kernel 串联。
- 标量 `let` 结果变量。
- 内存到标量 reduction。
- MLIR masked vector lowering。
- RVV assembly 和 RISC-V object 生成。

手动验证命令：

```bash
cd /home/zyz/zcomipler

./build/tools/zc/zc examples/complex_vector_pipeline.zc --emit-ast
./build/tools/zc/zc examples/complex_vector_pipeline.zc --emit-mlir \
  > /tmp/complex_vector_pipeline.mlir
/home/zyz/mlir/build/bin/mlir-opt /tmp/complex_vector_pipeline.mlir \
  -o /tmp/complex_vector_pipeline.checked.mlir

./build/tools/zc/zc examples/complex_vector_pipeline.zc --emit-riscv-asm \
  > /tmp/complex_vector_pipeline.s
riscv64-linux-gnu-as -march=rv64gcv -mabi=lp64d \
  /tmp/complex_vector_pipeline.s -o /tmp/complex_vector_pipeline.o
riscv64-linux-gnu-objdump -d /tmp/complex_vector_pipeline.o
```

## 当前边界

- 控制流程序可以走 AST、LLVM IR、部分 RISC-V 文本路径，但 in-memory
  MLIRGen 对 `if/while` 仍是后续 phase 工作。
- formal MLIR/LLVM 到 RVV machine code 的路径仍受本地 LLVM/MLIR 工具链限制：
  `/home/zyz/mlir/build/bin/llc` 当前没有 RISC-V target。直接 RVV reference
  backend 是当前可验证的 accelerator 输出路径。


## Phase 30O Capability Addendum

The compiler can now compile the first masked arithmetic example:

```zc
func masked_add_gt(mask_lhs: ptr<i32>, mask_rhs: ptr<i32>, a: ptr<i32>, b: ptr<i32>, passthrough: ptr<i32>, out: ptr<i32>, n: i32) -> i32 {
  vector_mask_gt m0, mask_lhs, mask_rhs, n;
  vector_masked_add out, a, b, m0, passthrough, n;
  return 0;
}
```

Supported output paths for this example are token dump, AST dump, MLIR, and
direct RVV assembly. The direct RVV path emits a compare mask in `v0`, masked
`vadd.vv`, `vmerge.vvm` passthrough selection, and `vse32.v`. The kernel is now
covered by host correctness and by the QEMU RVV execution harness.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_add_gt.zc --emit-ast
./build/tools/zc/zc examples/vector_masked_add_gt.zc --emit-mlir
./build/tools/zc/zc examples/vector_masked_add_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```


## Phase 30P Capability Addendum

`vector_masked_add` can now be driven by the complete current mask predicate
family: `vector_mask_lt`, `vector_mask_le`, `vector_mask_gt`, `vector_mask_ge`,
`vector_mask_eq`, `vector_mask_ne`, `vector_mask_ult`, `vector_mask_ule`,
`vector_mask_ugt`, and `vector_mask_uge`. Each form has token, AST, MLIR, direct
RVV assembly, host correctness, objdump, and QEMU runtime validation.

Example command:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_add_ult.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```


## Phase 30Q Capability Addendum

Masked arithmetic now has a generic binary AST and supports `vector_masked_add`,
`vector_masked_sub`, and `vector_masked_mul` in the direct RVV reference backend.
The new sub/mul slices are validated with MLIR, objdump, host correctness, and
QEMU execution using `vector_mask_gt`.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_sub_gt.zc --emit-riscv-asm
./build/tools/zc/zc examples/vector_masked_mul_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Phase 30R Capability Addendum

The compiler can now compile `vector_masked_store` as the first masked memory consumer:

```zc
func masked_store_gt(mask_lhs: ptr<i32>, mask_rhs: ptr<i32>, values: ptr<i32>, out: ptr<i32>, n: i32) -> i32 {
  vector_mask_gt m0, mask_lhs, mask_rhs, n;
  vector_masked_store out, values, m0, n;
  return 0;
}
```

The MLIR path combines the vector tail mask and compare mask with `arith.andi` before `vector.transfer_write`. The direct RVV path emits `vse32.v ..., v0.t`, so false mask lanes preserve the old destination memory value instead of selecting a passthrough vector.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_store_gt.zc --emit-ast
./build/tools/zc/zc examples/vector_masked_store_gt.zc --emit-mlir
./build/tools/zc/zc examples/vector_masked_store_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Phase 30S Capability Addendum

`vector_masked_load` is now supported as a masked memory read with explicit passthrough:

```zc
func masked_load_gt(mask_lhs: ptr<i32>, mask_rhs: ptr<i32>, input: ptr<i32>, passthrough: ptr<i32>, out: ptr<i32>, n: i32) -> i32 {
  vector_mask_gt m0, mask_lhs, mask_rhs, n;
  vector_masked_load out, input, m0, passthrough, n;
  return 0;
}
```

The direct RVV path emits masked `vle32.v ..., v0.t`, then `vmerge.vvm` with the passthrough vector. This avoids relying on masked-off destination lane behavior while the active profile uses `ta, ma`.


## Phase 31T Capability Addendum

The compiler now supports the first compiler-owned matrix multiply operation:

```zc
func matmul_i32(c: ptr<i32>, a: ptr<i32>, b: ptr<i32>, rows: i32, cols: i32, inner: i32) -> i32 {
  matrix_multiply c, a, b, rows, cols, inner;
  return 0;
}
```

This v1 MMA slice treats `a` as row-major `rows x inner`, `b` as row-major
`inner x cols`, and `c` as row-major `rows x cols`. It overwrites `c` with
wrapping `i32` dot products. Supported paths are token dump, AST dump, MLIR API
generation with nested `scf.for`, direct scalar RISC-V assembly, assembler
validation, and QEMU runtime correctness validation.

Manual visible QEMU demo for `matrix_multiply`:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/matrix_multiply.zc --emit-riscv-asm > /tmp/matrix_multiply.s
riscv64-linux-gnu-gcc -static -no-pie -march=rv64gcv -mabi=lp64d /tmp/matrix_multiply.s test/qemu/matrix_multiply_harness.c -o /tmp/matrix_multiply
/home/qemu/qemu/build-riscv64-user/qemu-riscv64 -cpu max /tmp/matrix_multiply
# matrix_multiply demo 2x3 * 3x2 = [58 64; 139 154]
```

This is correct scalar matrix multiply support, not RVV-optimized matmul yet.
Future phases should add transposed/packed-B or strided/indexed RVV memory forms
before marking matrix multiply as an RVV accelerator kernel.


## Phase 31U Capability Addendum

The compiler now supports an RVV-friendly packed-B matrix multiply operation:

```zc
func matmul_packed_b_i32(c: ptr<i32>, a: ptr<i32>, packed_b: ptr<i32>, rows: i32, cols: i32, inner: i32) -> i32 {
  matrix_multiply_packed_b c, a, packed_b, rows, cols, inner;
  return 0;
}
```

`packed_b` has shape `cols x inner` and stores each original `B` column as a
contiguous row. The direct RVV backend lowers each output dot product with
unit-stride `vle32.v` loads, `vmul.vv`, and `vredsum.vs`. Token, AST, MLIR,
assembler/objdump, and QEMU runtime validation are covered.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/matrix_multiply_packed_b.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```


## Phase 37A Capability Addendum

Additional programs now compile and run through the direct RISC-V/RVV backend:

- `examples/matrix_pack_b_then_multiply.zc`
- `examples/vector_add_i16.zc`
- `examples/vector_add_i16_m2.zc`
- `examples/vector_add_i8.zc`
- `examples/vector_add_i64.zc`
- `examples/vector_copy_i8.zc`
- `examples/vector_copy_i64.zc`
- `examples/vector_mul_i8.zc`
- `examples/vector_mul_i64.zc`
- `examples/vector_reduce_add_i8.zc`
- `examples/vector_reduce_add_i64.zc`
- `examples/vector_scale_i8.zc`
- `examples/vector_scale_i64.zc`
- `examples/vector_select_i8_gt.zc`
- `examples/vector_select_i64_gt.zc`
- `examples/vector_strided_load.zc`
- `examples/vector_indexed_load.zc`
- `examples/vector_strided_store.zc`
- `examples/vector_indexed_store.zc`
- `examples/vector_masked_strided_load.zc`
- `examples/vector_masked_indexed_load.zc`
- `examples/vector_masked_strided_store.zc`
- `examples/vector_masked_indexed_store.zc`
- `examples/vector_mask_logical.zc`
- `examples/vector_widen_add_i16_i32.zc`

The most useful manual runtime check for the newest slice is:

```bash
cd /home/zyz/zcomipler
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

The new QEMU harness prints:

```text
vector SEW demo n=31 i8/i16/i64 add/copy/mul/reduce/scale/select passed
vector memory/mask/widen demo passed n=17 store_n=31 masked_nonunit_n=31
```

Phase 37A also provides a repeatable formal lowering probe:

```bash
cd /home/zyz/zcomipler
MLIR_BUILD=/home/zyz/mlir/build ./scripts/probe-formal-rvv-lowering.sh
```

Current probe status: `blocked_at_riscv_llc`.

## Phase 38A Capability Addendum

- `examples/vector_add_i16_m4.zc` now compiles and runs through QEMU.
- The active RVV profile records `m1`, `m2`, and `m4` LMUL support for the
  current validated add slices.
- The direct RVV backend records an explicit clobber policy: zcompiler-generated
  RVV helper calls own the vector register groups used by the emitted kernel.

## Phase 39A/39B Capability Addendum

- `examples/vector_strided_store.zc` now compiles to `vsse32.v` and runs under
  QEMU.
- `examples/vector_indexed_store.zc` now compiles to `vsuxei32.v` and runs under
  QEMU.
- Store validation checks full destination arrays so untouched and tail elements
  remain unchanged.

## Phase 39C Capability Addendum

Masked non-unit RVV memory is now supported for the current `ptr<i32>` subset:

```zc
vector_masked_strided_load out, input, stride, m0, passthrough, n;
vector_masked_indexed_load out, input, indices, m0, passthrough, n;
vector_masked_strided_store base, values, stride, m0, n;
vector_masked_indexed_store base, values, indices, m0, n;
```

The direct RVV backend emits `vlse32.v` / `vluxei32.v` for masked non-unit loads
and `vsse32.v` / `vsuxei32.v` for masked non-unit stores, all guarded by
`v0.t`. Masked loads explicitly merge with passthrough values; masked stores
preserve false lanes in memory.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_masked_strided_load.zc --emit-riscv-asm
./build/tools/zc/zc examples/vector_masked_indexed_store.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Phase 40A Capability Addendum

The compiler now validates `i8` and `i64` unit-stride vector slices:

```zc
vector_add c, a, b, n;
vector_copy out, input, n;
vector_select_gt out, lhs, rhs, true_values, false_values, n;
```

For `ptr<i8>` the direct RVV backend emits `e8`, `vle8.v`, and `vse8.v`. For
`ptr<i64>` it emits `e64`, `vle64.v`, and `vse64.v`. The QEMU harness checks
lengths `0, 1, 2, 3, 5, 8, 17, 31` and verifies tail preservation.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_add_i8.zc --emit-riscv-asm
./build/tools/zc/zc examples/vector_select_i64_gt.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Phase 40B1 Capability Addendum

`vector_mul` and `vector_scale` now use typed SEW selection in the direct RVV
backend for the validated `ptr<i8>`, `ptr<i32>`, and `ptr<i64>` unit-stride
slices. `ptr<i8>` emits `vle8.v`/`vse8.v`; `ptr<i64>` emits
`vle64.v`/`vse64.v`; both use `vmul.vv` or `vmul.vx`.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_mul_i8.zc --emit-riscv-asm
./build/tools/zc/zc examples/vector_scale_i64.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```

## Phase 40B2 Capability Addendum

`vector_reduce_add` now has validated `ptr<i8>` and `ptr<i64>` direct RVV slices.
The implemented policy is same-SEW reduction:

- `ptr<i8>` emits `e8`, `vle8.v`, and `vredsum.vs`; the extracted scalar is
  returned through the current `i32` scalar slot.
- `ptr<i64>` emits `e64`, `vle64.v`, and `vredsum.vs`; the accumulator/result
  must be `i64`.

Manual validation:

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/vector_reduce_add_i8.zc --emit-riscv-asm
./build/tools/zc/zc examples/vector_reduce_add_i64.zc --emit-riscv-asm
ctest --test-dir build -R qemu-riscv64 --output-on-failure
```
