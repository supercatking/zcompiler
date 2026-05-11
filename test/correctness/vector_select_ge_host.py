#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys


def choose(lhs, rhs, true_value, false_value):
    return true_value if lhs >= rhs else false_value


def scalar_vector_select_ge(lhs, rhs, true_values, false_values, length):
    return [choose(lhs[i], rhs[i], true_values[i], false_values[i]) for i in range(length)]


def masked_vector_select_ge(lhs, rhs, true_values, false_values, length, vector_width=4):
    sentinel = -999999
    output = [sentinel for _ in lhs]
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                index = base + lane
                output[index] = choose(lhs[index], rhs[index], true_values[index], false_values[index])
    return output


def require_contains(text, needles):
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError("generated MLIR is missing: " + ", ".join(missing))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_select_ge_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    source_file = source_root / "examples" / "vector_select_ge.zc"

    mlir = subprocess.check_output([str(zc_bin), str(source_file), "--emit-mlir"], text=True)
    checked_features = [
        "vector.create_mask",
        "vector.transfer_read",
        "vector.transfer_write",
        "arith.cmpi sge",
        "arith.select",
        "arith.minui",
    ]
    require_contains(mlir, checked_features)

    cases = []
    for length in [0, 1, 3, 4, 5, 7, 16, 17]:
        capacity = length + 4
        lhs = [i * 5 - 13 for i in range(capacity)]
        rhs = [lhs[i] if i % 3 == 0 else 17 - i * 3 for i in range(capacity)]
        true_values = [1000 + i * 11 for i in range(capacity)]
        false_values = [-1000 - i * 7 for i in range(capacity)]
        scalar = scalar_vector_select_ge(lhs, rhs, true_values, false_values, length)
        masked = masked_vector_select_ge(lhs, rhs, true_values, false_values, length)
        if masked[:length] != scalar:
            raise AssertionError(f"vector_select_ge mismatch for n={length}: {masked[:length]} != {scalar}")
        if any(value != -999999 for value in masked[length:]):
            raise AssertionError(f"tail lanes modified outside n={length}")
        cases.append({"length": length, "vector_width": 4, "active_tail_lanes": length % 4, "result": scalar})

    out_dir = source_root / "build" / "correctness"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "vector_select_ge_host.json").open("w", encoding="utf-8") as handle:
        json.dump({
            "schema_version": 1,
            "test_id": "vector_select_ge_host_correctness",
            "source_program": "examples/vector_select_ge.zc",
            "accelerator_profile": "profiles/rvv-default.json",
            "checked_mlir_features": checked_features,
            "cases": cases,
            "status": "passed",
        }, handle, indent=2)
        handle.write(chr(10))


if __name__ == "__main__":
    main()
