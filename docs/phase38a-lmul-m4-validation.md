# Phase 38A: LMUL m4 Validation

## Goal

Close the first LMUL `m4` execution gap by validating the existing
`vector_add_m4` lowering through lexer, parser, assembly golden, objdump-style
checks, and QEMU execution.

## Source Surface

```zc
func vadd_i16_m4(c: ptr<i16>, a: ptr<i16>, b: ptr<i16>, n: i32) -> i32 {
  vector_add_m4 c, a, b, n;
  return 0;
}
```

`vector_add_m4` keeps the same source semantics as `vector_add`: for
`0 <= i < n`, write `c[i] = (i16)(a[i] + b[i])`; elements after `n` are not
written. The current validated `m4` slice is intentionally limited to `ptr<i16>`
operands.

## Direct RVV Lowering

The direct backend emits:

- `vsetvli ..., e16, m4, ta, ma`
- `vle16.v v0, ...`
- `vle16.v v4, ...`
- `vadd.vv v8, v0, v4`
- `vse16.v v8, ...`

The chosen vector register groups start at `v0`, `v4`, and `v8`, matching RVV
LMUL=4 alignment rules.

## ABI / Clobber Policy

The current direct RVV helpers are leaf kernels. They clobber the caller-saved
integer temporaries selected by the backend and all vector registers used by the
kernel's register groups. Callers must not rely on vector register contents
surviving a zcompiler-generated RVV call.

This policy is recorded in `profiles/rvv-default.json` so future backend work can
replace the ad hoc direct emitter with a reusable RVV calling convention helper.

## Validation

Phase 38A adds:

- `examples/vector_add_i16_m4.zc`
- lexer/parser golden output
- direct RISC-V assembly golden output
- codegen checks for `e16, m4`, `vle16.v`, `vadd.vv`, and `vse16.v`
- QEMU runtime checks across the same i16 length set used by m1/m2

The larger LMUL feature remains marked as partial because only i16 vector add is
validated for `m2` and `m4`; other operations still use their existing LMUL
policies.
