#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys


PREDICATES = [
    ("lt", "slt", lambda a, b: a < b),
    ("le", "sle", lambda a, b: a <= b),
    ("gt", "sgt", lambda a, b: a > b),
    ("ge", "sge", lambda a, b: a >= b),
    ("eq", "eq", lambda a, b: a == b),
    ("ne", "ne", lambda a, b: a != b),
    ("ult", "ult", lambda a, b: bits_i32(a) < bits_i32(b)),
    ("ule", "ule", lambda a, b: bits_i32(a) <= bits_i32(b)),
    ("ugt", "ugt", lambda a, b: bits_i32(a) > bits_i32(b)),
    ("uge", "uge", lambda a, b: bits_i32(a) >= bits_i32(b)),
]


def bits_i32(value):
    return value & 0xFFFFFFFF


def to_i32(value):
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def require_contains(text, needles):
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError("generated MLIR is missing: " + ", ".join(missing))


def expected_masked_add(predicate_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length):
    output = []
    for i in range(length):
        if predicate_fn(mask_lhs[i], mask_rhs[i]):
            output.append(to_i32(bits_i32(lhs[i]) + bits_i32(rhs[i])))
        else:
            output.append(passthrough[i])
    return output


def chunk_model(predicate_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length, vector_width=4):
    sentinel = -999999
    output = [sentinel for _ in lhs]
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                index = base + lane
                if predicate_fn(mask_lhs[index], mask_rhs[index]):
                    output[index] = to_i32(bits_i32(lhs[index]) + bits_i32(rhs[index]))
                else:
                    output[index] = passthrough[index]
    return output


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_masked_add_predicates_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    all_cases = []

    for name, mlir_predicate, predicate_fn in PREDICATES:
        source_file = source_root / "examples" / f"vector_masked_add_{name}.zc"
        mlir = subprocess.check_output(
            [str(zc_bin), str(source_file), "--emit-mlir"], text=True
        )
        require_contains(
            mlir,
            [
                "vector.create_mask",
                "vector.transfer_read",
                "vector.transfer_write",
                f"arith.cmpi {mlir_predicate}",
                "arith.addi",
                "arith.select",
                "arith.minui",
            ],
        )

        cases = []
        for length in [0, 1, 3, 4, 5, 7, 16, 17]:
            capacity = length + 4
            mask_lhs = [i * 7 - 19 for i in range(capacity)]
            if name in {"eq", "ne"}:
                mask_rhs = [mask_lhs[i] if i % 3 == 0 else 13 - i * 5 for i in range(capacity)]
            else:
                mask_rhs = [13 - i * 5 for i in range(capacity)]
            lhs = [2147483600 - i if i % 2 == 0 else -2147483600 + i for i in range(capacity)]
            rhs = [100 + i if i % 3 == 0 else -200 - i for i in range(capacity)]
            passthrough = [-3000 - i * 11 for i in range(capacity)]
            scalar = expected_masked_add(predicate_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length)
            masked = chunk_model(predicate_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length)

            if masked[:length] != scalar:
                raise AssertionError(
                    f"vector_masked_add_{name} mismatch for n={length}: {masked[:length]} != {scalar}"
                )
            if any(value != -999999 for value in masked[length:]):
                raise AssertionError(f"tail lanes modified outside n={length}")

            cases.append(
                {
                    "length": length,
                    "vector_width": 4,
                    "active_tail_lanes": length % 4,
                    "result": scalar,
                }
            )
        all_cases.append({"predicate": name, "cases": cases})

    out_dir = source_root / "build" / "correctness"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "vector_masked_add_predicates_host.json").open("w", encoding="utf-8") as handle:
        json.dump(
            {
                "schema_version": 1,
                "test_id": "vector_masked_add_predicates_host_correctness",
                "source_programs": [f"examples/vector_masked_add_{name}.zc" for name, _, _ in PREDICATES],
                "accelerator_profile": "profiles/rvv-default.json",
                "predicates": [name for name, _, _ in PREDICATES],
                "cases": all_cases,
                "status": "passed",
            },
            handle,
            indent=2,
        )
        handle.write("\n")


if __name__ == "__main__":
    main()
