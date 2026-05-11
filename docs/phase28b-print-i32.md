# Phase 28B: Built-in `print_i32`

## Goal

Add the smallest useful terminal-output surface to the toy language so a
compiled RISC-V binary can print a computed integer value when run under QEMU.

## Source Syntax

```zc
print_i32 expr;
```

`print_i32` is a statement. It evaluates an `i32` expression, prints the value
as signed decimal text, and appends a newline.

Example:

```zc
func main() -> i32 {
  let result = 100 + 120;
  print_i32 result;
  return result;
}
```

## Lowering Design

The first implementation is intentionally narrow:

- Lexer recognizes `print_i32` as a reserved keyword.
- Parser produces `PrintI32StmtAST`.
- AST dumps show `PrintI32Stmt`.
- The text RISC-V backend emits a call to `zc_print_i32`.
- The backend emits `zc_print_i32` once per module when needed.
- The runtime helper converts signed `i32` to decimal text on the stack.
- Output uses the RISC-V Linux `write` syscall:

```text
a0 = 1      # stdout
a1 = buffer
a2 = length
a7 = 64     # write
ecall
```

The call site saves and restores caller-saved argument and temporary registers
plus `ra`, because the current hand-written RISC-V backend stores source values
directly in those registers.

## Current Boundary

`print_i32` is currently a RISC-V runtime feature. MLIR API generation for this
statement is intentionally deferred until the compiler has a clearer runtime
ABI model for external calls or builtin side effects.

## Validation

```bash
cd /home/zyz/zcomipler
./build/tools/zc/zc examples/print_i32.zc --emit-tokens
./build/tools/zc/zc examples/print_i32.zc --emit-ast
./build/tools/zc/zc examples/print_i32.zc --emit-riscv-asm
ctest --test-dir build --output-on-failure
```
