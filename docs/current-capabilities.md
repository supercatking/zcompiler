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
  - `vector_reduce_add sum, a, n;`

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

在当前 WSL 环境中，`/home/qemu/qemu/build-riscv64-user/qemu-riscv64`
已经可以运行 zcompiler 生成并链接出的 RISC-V64 Linux ELF。CTest 中的
`qemu-riscv64` target 会验证 `print_i32` stdout 和 RVV pipeline 运行结果。

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
