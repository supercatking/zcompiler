#!/usr/bin/env python3
import argparse
from pathlib import Path

from manifest import load_and_validate


HARNESS_TEMPLATE = '''#include <stdint.h>

{kernel_comments}
extern int complex_vector_pipeline(int *a, int *b, int *tmp, int *out,
                                   int n, int factor);
extern int copy_then_sum(int *a, int *out, int n);
extern int vmul(int *a, int *b, int *c, int n);
extern int masked_add_lt(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_le(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_gt(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_ge(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_eq(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_ne(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_ult(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_ule(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_ugt(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_add_uge(int *mask_lhs, int *mask_rhs, int *a, int *b,
                           int *passthrough, int *out, int n);
extern int masked_sub_gt(int *mask_lhs, int *mask_rhs, int *a, int *b,
                         int *passthrough, int *out, int n);
extern int masked_mul_gt(int *mask_lhs, int *mask_rhs, int *a, int *b,
                         int *passthrough, int *out, int n);
extern int masked_store_gt(int *mask_lhs, int *mask_rhs, int *values,
                           int *out, int n);
extern int masked_load_gt(int *mask_lhs, int *mask_rhs, int *input,
                          int *passthrough, int *out, int n);
extern int select_lt(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_le(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_gt(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_ge(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_eq(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_ne(int *a, int *b, int *true_values, int *false_values,
                     int *out, int n);
extern int select_ult(int *a, int *b, int *true_values, int *false_values,
                      int *out, int n);
extern int select_ule(int *a, int *b, int *true_values, int *false_values,
                      int *out, int n);
extern int select_ugt(int *a, int *b, int *true_values, int *false_values,
                      int *out, int n);
extern int select_uge(int *a, int *b, int *true_values, int *false_values,
                      int *out, int n);

static uint32_t bits_i32(int value) {{ return (uint32_t)(int32_t)value; }}

static uint32_t add_i32_bits(int lhs, int rhs) {{
  return bits_i32(lhs) + bits_i32(rhs);
}}

static uint32_t mul_i32_bits(uint32_t lhs, int rhs) {{
  return lhs * bits_i32(rhs);
}}

{seed_helpers}
static int run_case(int n, int factor) {{
  int a[{capacity}];
  int b[{capacity}];
  int tmp[{capacity}];
  int out[{capacity}];
  int copied[{capacity}];
  int multiplied[{capacity}];
  int passthrough[{capacity}];
  int masked_added[{capacity}];
  int masked_stored[{capacity}];
  int masked_loaded[{capacity}];
  int true_values[{capacity}];
  int false_values[{capacity}];
  int selected_lt[{capacity}];
  int selected_le[{capacity}];
  int selected[{capacity}];
  int selected_ge[{capacity}];
  int eq_rhs[{capacity}];
  int selected_eq[{capacity}];
  int selected_ne[{capacity}];
  int selected_ult[{capacity}];
  int selected_ule[{capacity}];
  int selected_ugt[{capacity}];
  int selected_uge[{capacity}];

  for (int i = 0; i < {capacity}; ++i) {{
    a[i] = seed_a(i);
    b[i] = seed_b(i);
    tmp[i] = 0;
    out[i] = 0;
    copied[i] = 0;
    multiplied[i] = 0;
    passthrough[i] = -300000 - i * 19;
    masked_added[i] = 0;
    masked_stored[i] = -700000 - i * 23;
    masked_loaded[i] = -900000 - i * 37;
    true_values[i] = 100000 + i * 13;
    false_values[i] = -100000 - i * 17;
    selected_lt[i] = 0;
    selected_le[i] = 0;
    selected[i] = 0;
    selected_ge[i] = 0;
    eq_rhs[i] = (i % 3 == 0) ? a[i] : b[i];
    selected_eq[i] = 0;
    selected_ne[i] = 0;
    selected_ult[i] = 0;
    selected_ule[i] = 0;
    selected_ugt[i] = 0;
    selected_uge[i] = 0;
  }}

{pipeline_check}
{copy_check}
{mul_check}
{masked_add_check}
{masked_arithmetic_check}
{masked_store_check}
{masked_load_check}
{select_lt_check}
{select_le_check}
{select_check}
{select_ge_check}
{select_eq_check}
{select_ne_check}
{select_ult_check}
{select_ule_check}
{select_ugt_check}
{select_uge_check}
  return 0;
}}

static int factor_for_case(int i) {{
  switch (i % 4) {{
  case 0:
    return -3;
  case 1:
    return 5;
  case 2:
    return -7;
  default:
    return 9;
  }}
}}

int main(void) {{
  int lengths[] = {{{lengths}}};
  int count = sizeof(lengths) / sizeof(lengths[0]);

  for (int i = 0; i < count; ++i) {{
    int status = run_case(lengths[i], factor_for_case(i));
    if (status != 0)
      return 100 + i;
  }}

  return 0;
}}
'''


def indent(text, spaces=2):
    prefix = " " * spaces
    return "".join(prefix + line if line.strip() else line for line in text.splitlines(True))


def render_kernel_comments(kernel_checks):
    return "".join(
        f"/* {check['kernel']}: {check['check']} */\n" for check in kernel_checks
    )


def render_seed_helpers():
    return '''static int seed_a(int i) {
  switch (i % 6) {
  case 0:
    return 2147483600 - i;
  case 1:
    return -2147483600 + i;
  case 2:
    return -(i + 2);
  case 3:
    return i + 1;
  case 4:
    return 123456789 + i;
  default:
    return -76543210 - i;
  }
}

static int seed_b(int i) {
  switch (i % 6) {
  case 0:
    return 100 + i;
  case 1:
    return -200 - i;
  case 2:
    return 17 + i;
  case 3:
    return -19 - i;
  case 4:
    return 37 + i;
  default:
    return 29 + i;
  }
}

'''


def render_pipeline_check():
    return indent('''int sum = complex_vector_pipeline(a, b, tmp, out, n, factor);
uint32_t expected = 0;
for (int i = 0; i < n; ++i) {
  uint32_t tmp_expected = add_i32_bits(a[i], b[i]);
  uint32_t out_expected = mul_i32_bits(tmp_expected, factor);
  if (bits_i32(tmp[i]) != tmp_expected)
    return 10 + i;
  if (bits_i32(out[i]) != out_expected)
    return 30 + i;
  expected += out_expected;
}
if (bits_i32(sum) != expected)
  return 1;

''')


def render_copy_check():
    return indent('''int copy_sum = copy_then_sum(a, copied, n);
uint32_t expected_copy_sum = 0;
for (int i = 0; i < n; ++i) {
  if (bits_i32(copied[i]) != bits_i32(a[i]))
    return 50 + i;
  expected_copy_sum += bits_i32(a[i]);
}
if (bits_i32(copy_sum) != expected_copy_sum)
  return 2;

''')


def render_mul_check():
    return indent('''int mul_status = vmul(a, b, multiplied, n);
if (mul_status != 0)
  return 3;
for (int i = 0; i < n; ++i) {
  uint32_t mul_expected = bits_i32(a[i]) * bits_i32(b[i]);
  if (bits_i32(multiplied[i]) != mul_expected)
    return 70 + i;
}

''')



def render_masked_add_check():
    checks = [
        ("lt", "a", "b", "a[i] < b[i]", 18, 290),
        ("le", "a", "b", "a[i] <= b[i]", 19, 310),
        ("gt", "a", "b", "a[i] > b[i]", 20, 330),
        ("ge", "a", "b", "a[i] >= b[i]", 21, 350),
        ("eq", "a", "eq_rhs", "a[i] == eq_rhs[i]", 22, 370),
        ("ne", "a", "eq_rhs", "a[i] != eq_rhs[i]", 23, 390),
        ("ult", "a", "b", "bits_i32(a[i]) < bits_i32(b[i])", 24, 410),
        ("ule", "a", "b", "bits_i32(a[i]) <= bits_i32(b[i])", 25, 430),
        ("ugt", "a", "b", "bits_i32(a[i]) > bits_i32(b[i])", 26, 450),
        ("uge", "a", "b", "bits_i32(a[i]) >= bits_i32(b[i])", 27, 470),
    ]
    chunks = []
    for predicate, mask_lhs, mask_rhs, condition, status_base, result_base in checks:
        chunks.append(f'''int masked_add_{predicate}_status = masked_add_{predicate}({mask_lhs}, {mask_rhs}, a, b, passthrough, masked_added, n);
if (masked_add_{predicate}_status != 0)
  return {status_base};
for (int i = 0; i < n; ++i) {{
  uint32_t add_expected = add_i32_bits(a[i], b[i]);
  int expected = {condition} ? (int32_t)add_expected : passthrough[i];
  if (bits_i32(masked_added[i]) != bits_i32(expected))
    return {result_base} + i;
}}

''')
    return indent("".join(chunks))


def render_masked_arithmetic_check():
    return indent('''int masked_sub_status = masked_sub_gt(a, b, a, b, passthrough, masked_added, n);
if (masked_sub_status != 0)
  return 28;
for (int i = 0; i < n; ++i) {
  uint32_t sub_expected = bits_i32(a[i]) - bits_i32(b[i]);
  int expected = a[i] > b[i] ? (int32_t)sub_expected : passthrough[i];
  if (bits_i32(masked_added[i]) != bits_i32(expected))
    return 490 + i;
}

int masked_mul_status = masked_mul_gt(a, b, a, b, passthrough, masked_added, n);
if (masked_mul_status != 0)
  return 29;
for (int i = 0; i < n; ++i) {
  uint32_t mul_expected = bits_i32(a[i]) * bits_i32(b[i]);
  int expected = a[i] > b[i] ? (int32_t)mul_expected : passthrough[i];
  if (bits_i32(masked_added[i]) != bits_i32(expected))
    return 510 + i;
}

''')


def render_masked_store_check():
    return indent('''int masked_store_status = masked_store_gt(a, b, true_values, masked_stored, n);
if (masked_store_status != 0)
  return 30;
for (int i = 0; i < n; ++i) {
  int initial = -700000 - i * 23;
  int expected = a[i] > b[i] ? true_values[i] : initial;
  if (bits_i32(masked_stored[i]) != bits_i32(expected))
    return 530 + i;
}
for (int i = n; i < {capacity}; ++i) {
  int initial = -700000 - i * 23;
  if (bits_i32(masked_stored[i]) != bits_i32(initial))
    return 550 + i;
}

''')


def render_masked_load_check():
    return indent('''int masked_load_status = masked_load_gt(a, b, true_values, false_values, masked_loaded, n);
if (masked_load_status != 0)
  return 31;
for (int i = 0; i < n; ++i) {
  int expected = a[i] > b[i] ? true_values[i] : false_values[i];
  if (bits_i32(masked_loaded[i]) != bits_i32(expected))
    return 570 + i;
}
for (int i = n; i < {capacity}; ++i) {
  int initial = -900000 - i * 37;
  if (bits_i32(masked_loaded[i]) != bits_i32(initial))
    return 590 + i;
}

''')


def render_select_lt_check():
    return indent('''int select_lt_status = select_lt(a, b, true_values, false_values, selected_lt, n);
if (select_lt_status != 0)
  return 6;
for (int i = 0; i < n; ++i) {
  int expected = a[i] < b[i] ? true_values[i] : false_values[i];
  if (bits_i32(selected_lt[i]) != bits_i32(expected))
    return 130 + i;
}

''')


def render_select_le_check():
    return indent('''int select_le_status = select_le(a, b, true_values, false_values, selected_le, n);
if (select_le_status != 0)
  return 7;
for (int i = 0; i < n; ++i) {
  int expected = a[i] <= b[i] ? true_values[i] : false_values[i];
  if (bits_i32(selected_le[i]) != bits_i32(expected))
    return 150 + i;
}

''')

def render_select_check():
    return indent('''int select_status = select_gt(a, b, true_values, false_values, selected, n);
if (select_status != 0)
  return 4;
for (int i = 0; i < n; ++i) {
  int expected = a[i] > b[i] ? true_values[i] : false_values[i];
  if (bits_i32(selected[i]) != bits_i32(expected))
    return 90 + i;
}

''')



def render_select_ge_check():
    return indent('''int select_ge_status = select_ge(a, b, true_values, false_values, selected_ge, n);
if (select_ge_status != 0)
  return 8;
for (int i = 0; i < n; ++i) {
  int expected = a[i] >= b[i] ? true_values[i] : false_values[i];
  if (bits_i32(selected_ge[i]) != bits_i32(expected))
    return 170 + i;
}

''')

def render_select_eq_check():
    return indent('int select_eq_status = select_eq(a, eq_rhs, true_values, false_values, selected_eq, n);\nif (select_eq_status != 0)\n  return 5;\nfor (int i = 0; i < n; ++i) {\n  int expected = a[i] == eq_rhs[i] ? true_values[i] : false_values[i];\n  if (bits_i32(selected_eq[i]) != bits_i32(expected))\n    return 110 + i;\n}\n\n')



def render_select_ne_check():
    return indent('''int select_ne_status = select_ne(a, eq_rhs, true_values, false_values, selected_ne, n);
if (select_ne_status != 0)
  return 9;
for (int i = 0; i < n; ++i) {
  int expected = a[i] != eq_rhs[i] ? true_values[i] : false_values[i];
  if (bits_i32(selected_ne[i]) != bits_i32(expected))
    return 190 + i;
}

''')


def render_select_ult_check():
    return indent('''int select_ult_status = select_ult(a, b, true_values, false_values, selected_ult, n);
if (select_ult_status != 0)
  return 14;
for (int i = 0; i < n; ++i) {
  int expected = bits_i32(a[i]) < bits_i32(b[i]) ? true_values[i] : false_values[i];
  if (bits_i32(selected_ult[i]) != bits_i32(expected))
    return 210 + i;
}

''')


def render_select_ule_check():
    return indent('''int select_ule_status = select_ule(a, b, true_values, false_values, selected_ule, n);
if (select_ule_status != 0)
  return 15;
for (int i = 0; i < n; ++i) {
  int expected = bits_i32(a[i]) <= bits_i32(b[i]) ? true_values[i] : false_values[i];
  if (bits_i32(selected_ule[i]) != bits_i32(expected))
    return 230 + i;
}

''')


def render_select_ugt_check():
    return indent('''int select_ugt_status = select_ugt(a, b, true_values, false_values, selected_ugt, n);
if (select_ugt_status != 0)
  return 16;
for (int i = 0; i < n; ++i) {
  int expected = bits_i32(a[i]) > bits_i32(b[i]) ? true_values[i] : false_values[i];
  if (bits_i32(selected_ugt[i]) != bits_i32(expected))
    return 250 + i;
}

''')


def render_select_uge_check():
    return indent('''int select_uge_status = select_uge(a, b, true_values, false_values, selected_uge, n);
if (select_uge_status != 0)
  return 17;
for (int i = 0; i < n; ++i) {
  int expected = bits_i32(a[i]) >= bits_i32(b[i]) ? true_values[i] : false_values[i];
  if (bits_i32(selected_uge[i]) != bits_i32(expected))
    return 270 + i;
}

''')

def render_harness(data):
    rvv = data["rvv_execution"]
    lengths = rvv["lengths"]
    capacity = max(max(lengths), 1)
    return HARNESS_TEMPLATE.format(
        kernel_comments=render_kernel_comments(rvv["kernel_checks"]),
        seed_helpers=render_seed_helpers(),
        pipeline_check=render_pipeline_check(),
        copy_check=render_copy_check(),
        mul_check=render_mul_check(),
        masked_add_check=render_masked_add_check(),
        masked_arithmetic_check=render_masked_arithmetic_check(),
        masked_store_check=render_masked_store_check().replace("{capacity}", str(capacity)),
        masked_load_check=render_masked_load_check().replace("{capacity}", str(capacity)),
        select_lt_check=render_select_lt_check(),
        select_le_check=render_select_le_check(),
        select_check=render_select_check(),
        select_ge_check=render_select_ge_check(),
        select_eq_check=render_select_eq_check(),
        select_ne_check=render_select_ne_check(),
        select_ult_check=render_select_ult_check(),
        select_ule_check=render_select_ule_check(),
        select_ugt_check=render_select_ugt_check(),
        select_uge_check=render_select_uge_check(),
        capacity=capacity,
        lengths=", ".join(str(length) for length in lengths),
    )


def main():
    parser = argparse.ArgumentParser(description="Generate QEMU RVV C harness")
    parser.add_argument("manifest")
    parser.add_argument("output")
    args = parser.parse_args()

    data = load_and_validate(args.manifest)
    Path(args.output).write_text(render_harness(data), encoding="utf-8")


if __name__ == "__main__":
    main()
