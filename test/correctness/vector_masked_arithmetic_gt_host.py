#!/usr/bin/env python3
import json
import pathlib
import subprocess
import sys


OPS = [
    ("add", "arith.addi", lambda a, b: bits_i32(a) + bits_i32(b)),
    ("sub", "arith.subi", lambda a, b: bits_i32(a) - bits_i32(b)),
    ("mul", "arith.muli", lambda a, b: bits_i32(a) * bits_i32(b)),
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


def model(op_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length, vector_width=4):
    sentinel = -999999
    output = [sentinel for _ in lhs]
    for base in range(0, length, vector_width):
        active_lanes = min(length - base, vector_width)
        for lane in range(vector_width):
            if lane < active_lanes:
                index = base + lane
                if mask_lhs[index] > mask_rhs[index]:
                    output[index] = to_i32(op_fn(lhs[index], rhs[index]))
                else:
                    output[index] = passthrough[index]
    return output


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: vector_masked_arithmetic_gt_host.py <zc_bin> <source_root>")

    zc_bin = pathlib.Path(sys.argv[1])
    source_root = pathlib.Path(sys.argv[2])
    records = []
    for op, mlir_op, op_fn in OPS:
        source_file = source_root / "examples" / f"vector_masked_{op}_gt.zc"
        mlir = subprocess.check_output([str(zc_bin), str(source_file), "--emit-mlir"], text=True)
        require_contains(
            mlir,
            ["vector.create_mask", "vector.transfer_read", "vector.transfer_write", "arith.cmpi sgt", mlir_op, "arith.select", "arith.minui"],
        )
        cases = []
        for length in [0, 1, 3, 4, 5, 7, 16, 17]:
            capacity = length + 4
            mask_lhs = [i * 7 - 19 for i in range(capacity)]
            mask_rhs = [13 - i * 5 for i in range(capacity)]
            lhs = [2147483600 - i if i % 2 == 0 else -2147483600 + i for i in range(capacity)]
            rhs = [100 + i if i % 3 == 0 else -200 - i for i in range(capacity)]
            passthrough = [-3000 - i * 11 for i in range(capacity)]
            output = model(op_fn, mask_lhs, mask_rhs, lhs, rhs, passthrough, length)
            expected = [to_i32(op_fn(lhs[i], rhs[i])) if mask_lhs[i] > mask_rhs[i] else passthrough[i] for i in range(length)]
            if output[:length] != expected:
                raise AssertionError(f"vector_masked_{op}_gt mismatch for n={length}")
            if any(value != -999999 for value in output[length:]):
                raise AssertionError(f"tail lanes modified outside n={length}")
            cases.append({"length": length, "result": expected})
        records.append({"op": op, "cases": cases})
    out_dir = source_root / "build" / "correctness"
    out_dir.mkdir(parents=True, exist_ok=True)
    with (out_dir / "vector_masked_arithmetic_gt_host.json").open("w", encoding="utf-8") as handle:
        json.dump({"schema_version": 1, "test_id": "vector_masked_arithmetic_gt_host_correctness", "source_programs": [f"examples/vector_masked_{op}_gt.zc" for op, _, _ in OPS], "accelerator_profile": "profiles/rvv-default.json", "ops": [op for op, _, _ in OPS], "cases": records, "status": "passed"}, handle, indent=2)
        handle.write("\n")


if __name__ == "__main__":
    main()
