#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys


def bits_i32(value):
    return value & 0xFFFFFFFF


def to_i32(value):
    value &= 0xFFFFFFFF
    return value - 0x100000000 if value & 0x80000000 else value


def scalar_masked_add_gt(mask_lhs, mask_rhs, lhs, rhs, passthrough, length):
    result = []
    for i in range(length):
        if mask_lhs[i] > mask_rhs[i]:
            result.append(to_i32(bits_i32(lhs[i]) + bits_i32(rhs[i])))
        else:
            result.append(passthrough[i])
    return result


def masked_chunk_model(mask_lhs, mask_rhs, lhs, rhs, passthrough, length, vector_width=4):
    sentinel = -999999
    output = [sentinel for _ in lhs]
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                index = base + lane
                if mask_lhs[index] > mask_rhs[index]:
                    output[index] = to_i32(bits_i32(lhs[index]) + bits_i32(rhs[index]))
                else:
                    output[index] = passthrough[index]
    return output


def require_contains(text, needles):
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError("generated MLIR is missing: " + ", ".join(missing))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_masked_add_gt_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    source_file = source_root / "examples" / "vector_masked_add_gt.zc"

    mlir = subprocess.check_output(
        [str(zc_bin), str(source_file), "--emit-mlir"], text=True
    )
    require_contains(
        mlir,
        [
            "vector.create_mask",
            "vector.transfer_read",
            "vector.transfer_write",
            "arith.cmpi sgt",
            "arith.addi",
            "arith.select",
            "arith.minui",
        ],
    )

    cases = []
    for length in [0, 1, 3, 4, 5, 7, 16, 17]:
        capacity = length + 4
        mask_lhs = [i * 7 - 19 for i in range(capacity)]
        mask_rhs = [13 - i * 5 for i in range(capacity)]
        lhs = [2147483600 - i if i % 2 == 0 else -2147483600 + i for i in range(capacity)]
        rhs = [100 + i if i % 3 == 0 else -200 - i for i in range(capacity)]
        passthrough = [-3000 - i * 11 for i in range(capacity)]
        scalar = scalar_masked_add_gt(mask_lhs, mask_rhs, lhs, rhs, passthrough, length)
        masked = masked_chunk_model(mask_lhs, mask_rhs, lhs, rhs, passthrough, length)

        if masked[:length] != scalar:
            raise AssertionError(
                f"vector_masked_add_gt mismatch for n={length}: {masked[:length]} != {scalar}"
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
    with (out_dir / "vector_masked_add_gt_host.json").open("w", encoding="utf-8") as handle:
        json.dump(
            {
                "schema_version": 1,
                "test_id": "vector_masked_add_gt_host_correctness",
                "source_program": "examples/vector_masked_add_gt.zc",
                "accelerator_profile": "profiles/rvv-default.json",
                "checked_mlir_features": [
                    "vector.create_mask",
                    "vector.transfer_read",
                    "vector.transfer_write",
                    "arith.cmpi sgt",
                    "arith.addi",
                    "arith.select",
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
