# Phase 40C1: Typed Strided Memory SEW

Phase 40C1 extends the direct RVV reference backend so unmasked strided memory
uses the pointee type of typed buffers instead of the old fixed `i32` path.

## Goal

Validate `vector_strided_load` and `vector_strided_store` for:

- `ptr<i8>`
- `ptr<i16>`
- `ptr<i32>`
- `ptr<i64>`

The source stride remains element-based. The backend converts it to a byte
stride before emitting RVV assembly.

## Source Syntax

```zc
vector_strided_load out, input, stride, n;
vector_strided_store base, values, stride, n;
```

Semantics:

- `vector_strided_load out, input, stride, n` reads
  `out[i] = input[i * stride]`.
- `vector_strided_store base, values, stride, n` writes
  `base[i * stride] = values[i]`.
- `stride` is counted in elements, not bytes.
- `n` is counted in elements and is lowered through the normal `vsetvli`
  remaining-length loop.

## Direct RVV Mapping

The direct RVV backend now derives SEW from the buffer element type:

| Source element | Load instruction | Store instruction | vtype |
| --- | --- | --- | --- |
| `i8` | `vlse8.v` | `vsse8.v` | `e8, m1, ta, ma` |
| `i16` | `vlse16.v` | `vsse16.v` | `e16, m1, ta, ma` |
| `i32` | `vlse32.v` | `vsse32.v` | `e32, m1, ta, ma` |
| `i64` | `vlse64.v` | `vsse64.v` | `e64, m1, ta, ma` |

For `i8`, element offsets already equal byte offsets. For wider elements, the
backend shifts the element stride and loop index by `log2(SEW / 8)`.

## Validation

Phase 40C1 adds examples, lexer/parser goldens, RISC-V assembly goldens,
objdump checks, and QEMU execution coverage for:

- `examples/vector_strided_load_i8.zc`
- `examples/vector_strided_load_i16.zc`
- `examples/vector_strided_load_i64.zc`
- `examples/vector_strided_store_i8.zc`
- `examples/vector_strided_store_i16.zc`
- `examples/vector_strided_store_i64.zc`

The QEMU harness covers lengths:

```text
0, 1, 2, 3, 5, 8, 17, 31
```

It checks both the touched strided elements and the untouched destination/tail
lanes.

## Remaining Work

Indexed memory across all SEW values is intentionally deferred to Phase 40C2 so
the compiler can document EMUL, offset width, and vector register-group legality
before broadening `vluxei*.v` and `vsuxei*.v` forms.
