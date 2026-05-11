#!/usr/bin/env python3
import argparse
import json
import shlex
from pathlib import Path


def load_and_validate(path):
    with Path(path).open(encoding="utf-8") as handle:
        data = json.load(handle)

    if data.get("schema_version") != 1:
        raise SystemExit("manifest schema_version must be 1")
    if not data.get("qemu_cpu"):
        raise SystemExit("manifest qemu_cpu must not be empty")

    print_i32 = data.get("print_i32")
    if not isinstance(print_i32, dict):
        raise SystemExit("manifest print_i32 must be an object")
    if not isinstance(print_i32.get("expected_stdout"), str):
        raise SystemExit("print_i32.expected_stdout must be a string")
    if not isinstance(print_i32.get("expected_exit_status"), int):
        raise SystemExit("print_i32.expected_exit_status must be an integer")

    scalar_i32_wrap = data.get("scalar_i32_wrap")
    if not isinstance(scalar_i32_wrap, dict):
        raise SystemExit("manifest scalar_i32_wrap must be an object")
    if not isinstance(scalar_i32_wrap.get("source"), str):
        raise SystemExit("scalar_i32_wrap.source must be a string")
    if not isinstance(scalar_i32_wrap.get("expected_stdout"), str):
        raise SystemExit("scalar_i32_wrap.expected_stdout must be a string")
    if not isinstance(scalar_i32_wrap.get("expected_exit_status"), int):
        raise SystemExit("scalar_i32_wrap.expected_exit_status must be an integer")
    instructions = scalar_i32_wrap.get("expected_instructions")
    if not instructions or not all(isinstance(item, str) for item in instructions):
        raise SystemExit("scalar_i32_wrap.expected_instructions must be a string list")

    rvv = data.get("rvv_execution")
    if not isinstance(rvv, dict):
        raise SystemExit("manifest rvv_execution must be an object")

    sources = rvv.get("sources")
    if not sources or not all(isinstance(source, str) for source in sources):
        raise SystemExit("rvv_execution.sources must be a non-empty string list")

    lengths = rvv.get("lengths")
    if not lengths or not all(isinstance(length, int) for length in lengths):
        raise SystemExit("rvv_execution.lengths must be a non-empty integer list")
    if any(length < 0 for length in lengths):
        raise SystemExit("rvv_execution.lengths must be non-negative")
    if len(lengths) != len(set(lengths)):
        raise SystemExit("rvv_execution.lengths must not contain duplicates")

    kernels = rvv.get("validated_kernels")
    if not kernels or not all(isinstance(kernel, str) for kernel in kernels):
        raise SystemExit("rvv_execution.validated_kernels must be a non-empty string list")

    checks = rvv.get("kernel_checks")
    if not checks or not all(isinstance(check, dict) for check in checks):
        raise SystemExit("rvv_execution.kernel_checks must be a non-empty object list")
    check_names = [check.get("kernel") for check in checks]
    if sorted(check_names) != sorted(kernels):
        raise SystemExit("kernel_checks must describe every validated kernel exactly once")
    for check in checks:
        if not check.get("function") or not check.get("check"):
            raise SystemExit("each kernel check needs function and check fields")

    integer_semantics = rvv.get("integer_semantics")
    if not isinstance(integer_semantics, dict):
        raise SystemExit("rvv_execution.integer_semantics must be an object")
    if integer_semantics.get("element_type") != "i32":
        raise SystemExit("rvv_execution.integer_semantics.element_type must be i32")
    if not integer_semantics.get("signed_test_policy"):
        raise SystemExit("rvv_execution.integer_semantics.signed_test_policy is required")
    if not integer_semantics.get("overflow_policy"):
        raise SystemExit("rvv_execution.integer_semantics.overflow_policy is required")

    return data


def emit_shell_env(data, output_path):
    rvv = data["rvv_execution"]
    print_i32 = data["print_i32"]
    scalar_i32_wrap = data["scalar_i32_wrap"]
    lengths = rvv["lengths"]
    capacity = max(max(lengths), 1)

    with Path(output_path).open("w", encoding="utf-8") as handle:
        handle.write(f"qemu_cpu={shlex.quote(data['qemu_cpu'])}\n")
        handle.write(
            f"print_expected_stdout={shlex.quote(print_i32['expected_stdout'])}\n"
        )
        handle.write(f"print_expected_status={print_i32['expected_exit_status']}\n")
        handle.write(f"scalar_wrap_source={shlex.quote(scalar_i32_wrap['source'])}\n")
        handle.write(
            f"scalar_wrap_expected_stdout={shlex.quote(scalar_i32_wrap['expected_stdout'])}\n"
        )
        handle.write(
            f"scalar_wrap_expected_status={scalar_i32_wrap['expected_exit_status']}\n"
        )
        handle.write(
            "rvv_lengths_c="
            + shlex.quote(", ".join(str(length) for length in lengths))
            + "\n"
        )
        handle.write(f"rvv_capacity={capacity}\n")


def main():
    parser = argparse.ArgumentParser(description="Validate qemu RVV manifest")
    parser.add_argument("manifest")
    parser.add_argument("--emit-shell-env")
    args = parser.parse_args()

    data = load_and_validate(args.manifest)
    if args.emit_shell_env:
        emit_shell_env(data, args.emit_shell_env)


if __name__ == "__main__":
    main()
