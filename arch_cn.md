# zcompiler 架构设计

`zcompiler` 是一个从 toy compiler 开始逐步演进的编译器项目。当前目标是先跑通一个小语言到 AST、MLIR、LLVM IR、RISC-V assembly 的完整链路；长期目标是发展成一个面向 RISC-V RVV accelerator 的 AI-assisted compiler。

## 架构图

![zcompiler architecture](arch.svg)

这张图描述的是当前项目的整体结构：

- 前端负责理解 `.zc` 源码，包括 lexer、parser 和 AST。
- AST 是当前最核心的中间数据结构，后续 codegen 都从 AST 读取程序结构。
- `CodeGen` 模块从 AST 生成多种输出：标准 MLIR、`zc` dialect MLIR 表面形式、lowered MLIR、LLVM IR 和 RISC-V assembly。
- RVV 和 AI workflow 目前是设计方向，已经有文档记录，为后续 accelerator compiler 做准备。

## 1. 总体流水线

当前编译路径如下：

```text
source .zc
  -> Lexer
  -> Token stream
  -> Parser
  -> AST
  -> CodeGen
  -> MLIR / LLVM IR / RISC-V assembly
```

更详细一点：

```text
examples/hello.zc
  -> zc::Lexer
  -> std::vector<Token>
  -> zc::Parser
  -> ModuleAST
  -> CodeGen
  -> --emit-ast
  -> --emit-mlir
  -> --emit-zc-mlir
  -> --emit-lowered-mlir
  -> --emit-llvm
  -> --emit-riscv-asm
```

## 2. Frontend

Frontend 由 lexer 和 parser 两部分组成。

### Lexer

Lexer 负责把源码字符串转换成 token stream。

代码位置：

```text
include/zcompiler/Lexer/
lib/Lexer/
```

当前支持：

- 关键字：`func`、`let`、`return`、`if`、`else`、`while`、`i32`
- 标识符
- 整数
- 算术运算符：`+`、`-`、`*`、`/`
- 比较运算符：`<`、`<=`、`>`、`>=`、`==`、`!=`
- 分隔符：`(`、`)`、`{`、`}`、`;`
- 箭头：`->`
- 单行注释：`//`

测试命令：

```bash
./build/tools/zc/zc examples/hello.zc --emit-tokens
```

### Parser

Parser 负责消费 token stream，并构建 AST。

代码位置：

```text
include/zcompiler/Parser/
lib/Parser/
```

当前 parser 是 recursive descent parser，并带有表达式优先级解析。它支持：

- function declaration
- `let` statement
- `return` statement
- `if` / `else`
- `while`
- 算术表达式
- 比较表达式

测试命令：

```bash
./build/tools/zc/zc examples/hello.zc --emit-ast
./build/tools/zc/zc examples/control.zc --emit-ast
./build/tools/zc/zc examples/while.zc --emit-ast
```

## 3. AST

AST 是 parser 的输出，也是 codegen 的输入。

代码位置：

```text
include/zcompiler/AST/
lib/AST/
```

当前主要节点：

- `ModuleAST`
- `FunctionAST`
- `LetStmtAST`
- `ReturnStmtAST`
- `IfStmtAST`
- `WhileStmtAST`
- `IntegerExprAST`
- `VariableExprAST`
- `BinaryExprAST`

AST 使用 `std::unique_ptr` 表示父节点拥有子节点，所以整棵树的生命周期由 `ModuleAST` 自顶向下管理。

例如：

```zc
func main() -> i32 {
  let x = 1 + 2 * 3;
  return x;
}
```

会变成：

```text
ModuleAST
  FunctionAST main -> i32
    LetStmtAST x
      BinaryExprAST "+"
        IntegerExprAST 1
        BinaryExprAST "*"
          IntegerExprAST 2
          IntegerExprAST 3
    ReturnStmtAST
      VariableExprAST x
```

## 4. CodeGen

`CodeGen` 模块负责从 AST 生成不同目标文本。

代码位置：

```text
include/zcompiler/CodeGen/
lib/CodeGen/
```

当前支持的输出：

```bash
./build/tools/zc/zc examples/hello.zc --emit-mlir
./build/tools/zc/zc examples/hello.zc --emit-zc-mlir
./build/tools/zc/zc examples/hello.zc --emit-lowered-mlir
./build/tools/zc/zc examples/hello.zc --emit-llvm
./build/tools/zc/zc examples/hello.zc --emit-riscv-asm
```

### 标准 MLIR

`--emit-mlir` 输出标准 MLIR 表面形式：

```mlir
module {
  func.func @main() -> i32 {
    %0 = arith.constant 1 : i32
    return %0 : i32
  }
}
```

### zc Dialect MLIR

`--emit-zc-mlir` 输出项目自有 dialect 的文本表面形式：

```mlir
module {
  zc.func @main() -> i32 {
    %0 = zc.constant 1 : i32
    zc.return %0 : i32
  }
}
```

当前已经有 ODS 草案：

```text
include/zcompiler/Dialect/ZC/IR/ZCDialect.td
include/zcompiler/Dialect/ZC/IR/ZCOps.td
```

下一步可以把它升级为真正注册进 MLIR 的 dialect。

### Lowered MLIR

`--emit-lowered-mlir` 表示把 `zc` dialect 表面形式 lowering 到标准 MLIR。

当前是 AST-backed textual lowering。后续更正式的实现应该改成：

```text
zc dialect ops
  -> MLIR rewrite patterns
  -> func / arith / scf dialect
```

### LLVM IR

`--emit-llvm` 输出 LLVM IR：

```llvm
define i32 @main() {
entry:
  %0 = mul i32 2, 3
  %1 = add i32 1, %0
  ret i32 %1
}
```

测试里会使用 `llvm-as` 验证 LLVM IR 是否可解析。

### RISC-V Assembly

`--emit-riscv-asm` 输出 RISC-V assembly：

```asm
  .option nopic
  .text
  .globl main
main:
  li t0, 1
  li t1, 2
  add t2, t0, t1
  mv a0, t2
  ret
```

测试里会在可用时使用 `riscv64-linux-gnu-as` 验证 assembly。

## 5. 测试体系

测试入口：

```bash
ctest --test-dir build --output-on-failure
```

当前测试覆盖：

- lexer golden tests
- parser AST golden tests
- MLIR golden tests
- LLVM IR golden tests
- RISC-V assembly golden tests
- `mlir-opt` 可解析检查
- `llvm-as` 可解析检查
- 可选 `riscv64-linux-gnu-as` 汇编检查

## 6. RVV 方向

RVV 方向记录在：

```text
docs/rvv.md
```

计划路径：

```text
zc vector syntax
  -> zc vector ops
  -> MLIR vector dialect
  -> LLVM dialect / RVV intrinsics
  -> RISC-V RVV assembly
```

未来重点：

- vector load/store
- vector add/mul
- reduction
- `vsetvli`
- `vle32.v`
- `vse32.v`
- `vadd.vv`
- `vmul.vv`

## 7. AI-Assisted Compiler Workflow

AI-assisted workflow 记录在：

```text
docs/ai-workflow.md
```

基本原则：

- AI 可以提出 compiler pass、lowering、优化建议。
- 所有 AI 生成的 compiler 变更都必须经过测试。
- 重要变更需要保存 IR/assembly 对比。
- benchmark 和 prompt 都要记录，方便复现。

## 8. 当前架构状态

当前项目已经完成 toy compiler 的完整垂直切片：

```text
.zc source
  -> tokens
  -> AST
  -> MLIR
  -> LLVM IR
  -> RISC-V assembly
```

下一步最有价值的工程方向是：

- 把 `zc` dialect 从文本 surface 升级为真正 MLIR registered dialect。
- 使用 MLIR rewrite pattern 实现真正 lowering。
- 将 RISC-V assembly 路径接到 LLVM backend 或更正式的 target pipeline。
- 开始实现 RVV vector operation。

## 9. 后续 Phase 规划

对照最终目标：

```text
an AI self made compiler based on RISCV RVV accelerator
```

后续阶段规划如下：

- Phase 12：真正注册 `zc` MLIR dialect。
- Phase 13：使用 MLIR rewrite pattern 实现真实 lowering pass。
- Phase 14：使用 MLIR C++ API 从 AST 构造 in-memory MLIR module。
- Phase 15：通过 MLIR LLVM dialect 和 MLIR/LLVM 基础设施生成 LLVM IR。
- Phase 16：接入 LLVM RISC-V backend 生成 RISC-V assembly/object。
- Phase 17：扩展函数、函数调用、赋值和内存模型。
- Phase 18：添加 target-independent vector syntax 和 vector AST。
- Phase 19：把 vector operations lowering 到 MLIR vector dialect。
- Phase 20：把 vector operations lowering 到 RVV intrinsics 或 RVV assembly。
- Phase 21：添加 accelerator profile 和 benchmark runner。
- Phase 22：实现 AI-assisted optimization experiment loop。

## 10. 工程规则

后续开发必须遵守：

- 先考虑核心架构并更新文档，再动手写代码。
- 代码尽量结构化、可扩展、低耦合、容易读懂。
- Lexer、Parser、AST、Dialect、Lowering、Backend、Benchmark、AI workflow 要保持清晰边界。
- 架构变化时及时更新 UML、架构图或 workflow 图。
- 每个 phase 都要补充 testcase。
- 尽量完整验证；如果有难以立即修复的问题，记录到 `known_issue.md`。

下一步建议优先做 Phase 12。原因是它会把当前 `zc` dialect 从“文本表面形式”升级为真正 MLIR registered dialect，这是后续真实 lowering、LLVM pipeline 和 RVV lowering 的基础。
