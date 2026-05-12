#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def load_matrix(path):
    with Path(path).open(encoding="utf-8") as handle:
        data = json.load(handle)
    if data.get("schema_version") != 1:
        raise SystemExit("matrix schema_version must be 1")
    features = data.get("features")
    if not isinstance(features, list) or not features:
        raise SystemExit("matrix features must be a non-empty list")
    required = {"area", "feature", "status", "phase", "validation"}
    allowed = set(data.get("status_levels", []))
    for feature in features:
        missing = required - set(feature)
        if missing:
            raise SystemExit(f"feature is missing fields: {sorted(missing)}")
        if feature["status"] not in allowed:
            raise SystemExit(f"unknown status {feature['status']} for {feature['feature']}")
        if not isinstance(feature["validation"], list):
            raise SystemExit(f"validation must be a list for {feature['feature']}")
    return data


def render_markdown(data):
    lines = [
        "# Generated RVV 1.0 Compliance Matrix",
        "",
        "This file is generated from `profiles/rvv-compliance-matrix.json`.",
        "Do not edit the table by hand; update the JSON and regenerate it.",
        "",
        "| Area | Feature | Status | Phase | Validation |",
        "| --- | --- | --- | --- | --- |",
    ]
    for item in sorted(data["features"], key=lambda x: (x["area"], x["feature"])):
        validation = ", ".join(item["validation"]) if item["validation"] else "-"
        lines.append(
            f"| `{item['area']}` | `{item['feature']}` | `{item['status']}` | `{item['phase']}` | {validation} |"
        )
    counts = {}
    for item in data["features"]:
        counts[item["status"]] = counts.get(item["status"], 0) + 1
    lines += ["", "## Status Counts", ""]
    for status in data["status_levels"]:
        lines.append(f"- `{status}`: {counts.get(status, 0)}")
    lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate RVV compliance markdown")
    parser.add_argument("matrix", nargs="?", default="profiles/rvv-compliance-matrix.json")
    parser.add_argument("--output", default="docs/rvv-1.0-compliance-generated.md")
    args = parser.parse_args()

    data = load_matrix(args.matrix)
    output = Path(args.output)
    output.write_text(render_markdown(data), encoding="utf-8")
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
