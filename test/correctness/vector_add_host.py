#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys


def scalar_vector_add(lhs, rhs, length):
    return [lhs[i] + rhs[i] for i in range(length)]


def masked_vector_add(lhs, rhs, length, vector_width=4):
    sentinel = -999999
    output = [sentinel for _ in lhs]
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                output[base + lane] = lhs[base + lane] + rhs[base + lane]
    return output


def require_contains(text, needles):
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError("generated MLIR is missing: " + ", ".join(missing))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_add_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    source_file = source_root / "examples" / "vector_add.zc"

    mlir = subprocess.check_output(
        [str(zc_bin), str(source_file), "--emit-mlir"], text=True
    )
    require_contains(
        mlir,
        [
            "vector.create_mask",
            "vector.transfer_read",
            "vector.transfer_write",
            "arith.minui",
        ],
    )

    cases = []
    for length in [0, 1, 3, 4, 5, 7, 16, 17]:
        capacity = length + 4
        lhs = [i * 3 - 7 for i in range(capacity)]
        rhs = [11 - i * 2 for i in range(capacity)]
        scalar = scalar_vector_add(lhs, rhs, length)
        masked = masked_vector_add(lhs, rhs, length)

        if masked[:length] != scalar:
            raise AssertionError(
                f"vector_add mismatch for n={length}: {masked[:length]} != {scalar}"
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

    out_dir = source_root / "build" / "correctness"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "vector_add_host.json").open("w", encoding="utf-8") as handle:
        json.dump(
            {
                "schema_version": 1,
                "test_id": "vector_add_host_correctness",
                "source_program": "examples/vector_add.zc",
                "accelerator_profile": "profiles/rvv-default.json",
                "checked_mlir_features": [
                    "vector.create_mask",
                    "vector.transfer_read",
                    "vector.transfer_write",
                    "arith.minui",
                ],
                "cases": cases,
                "status": "passed",
            },
            handle,
            indent=2,
        )
        handle.write("\n")


if __name__ == "__main__":
    main()
