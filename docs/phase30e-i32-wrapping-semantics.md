# Phase 30E: i32 Wrapping Semantics

## Goal

Define the current arithmetic contract for the implemented RVV `i32` kernel
subset and validate it with QEMU bit-pattern checks.

## Current Policy

For the current RVV kernel subset, `i32` arithmetic is checked as wrapping modulo
`2^32`. The runtime harness compares `uint32_t` bit patterns instead of relying
on signed C overflow.

Covered kernels:

- `vector_add`
- `vector_scale`
- `vector_reduce_add`
- `vector_mul`

`vector_copy` is included in the same signed input matrix, but it does not
perform arithmetic.

## QEMU Coverage

`test/qemu/run.sh` now seeds inputs near positive and negative `i32` boundaries,
uses negative scale factors, and computes expected values with unsigned 32-bit
arithmetic. This checks the bit patterns produced by RVV add, multiply, and
reduction paths.

## Boundary

This policy currently applies to the implemented RVV kernel subset. A later
frontend-wide language semantics document should decide whether all scalar `i32`
source expressions use the same wrapping rule.
