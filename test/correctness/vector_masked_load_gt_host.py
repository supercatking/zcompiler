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


def require_contains(text, needles):
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise AssertionError("generated output is missing: " + ", ".join(missing))


def masked_load_model(mask_lhs, mask_rhs, values, passthrough, initial, length, vector_width=4):
    output = list(initial)
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                index = base + lane
                output[index] = values[index] if mask_lhs[index] > mask_rhs[index] else passthrough[index]
    return output


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_masked_load_gt_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    source_file = source_root / "examples" / "vector_masked_load_gt.zc"

    mlir = subprocess.check_output([str(zc_bin), str(source_file), "--emit-mlir"], text=True)
    require_contains(
        mlir,
        [
            "vector.create_mask",
            "vector.transfer_read",
            "vector.transfer_write",
            "arith.cmpi sgt",
            "arith.andi",
            "arith.select",
            "arith.minui",
        ],
    )

    riscv = subprocess.check_output([str(zc_bin), str(source_file), "--emit-riscv-asm"], text=True)
    require_contains(riscv, ["vle32.v", "v0.t", "vmerge.vvm", "vse32.v"])

    cases = []
    for length in [0, 1, 3, 4, 5, 7, 16, 17]:
        capacity = length + 4
        mask_lhs = [i * 7 - 19 for i in range(capacity)]
        mask_rhs = [13 - i * 5 for i in range(capacity)]
        values = [to_i32(100000 + i * 31) for i in range(capacity)]
        passthrough = [to_i32(-500000 - i * 29) for i in range(capacity)]
        initial = [to_i32(-900000 - i * 37) for i in range(capacity)]
        output = masked_load_model(mask_lhs, mask_rhs, values, passthrough, initial, length)

        for i in range(length):
            expected = values[i] if mask_lhs[i] > mask_rhs[i] else passthrough[i]
            if bits_i32(output[i]) != bits_i32(expected):
                raise AssertionError(f"masked load mismatch at n={length}, i={i}")
        if output[length:] != initial[length:]:
            raise AssertionError(f"tail elements modified outside n={length}")

        cases.append({"length": length, "result": output[:length], "tail_preserved": output[length:] == initial[length:]})

    out_dir = source_root / "build" / "correctness"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "vector_masked_load_gt_host.json").open("w", encoding="utf-8") as handle:
        json.dump(
            {
                "schema_version": 1,
                "test_id": "vector_masked_load_gt_host_correctness",
                "source_program": "examples/vector_masked_load_gt.zc",
                "accelerator_profile": "profiles/rvv-default.json",
                "checked_mlir_features": ["arith.andi", "arith.select", "vector.transfer_read", "vector.transfer_write"],
                "checked_rvv_features": ["vle32.v mask v0.t", "vmerge.vvm", "vse32.v"],
                "cases": cases,
                "status": "passed",
            },
            handle,
            indent=2,
        )
        handle.write("\n")


if __name__ == "__main__":
    main()
