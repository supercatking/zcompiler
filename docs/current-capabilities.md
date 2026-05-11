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
