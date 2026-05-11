#!/usr/bin/env bash
set -euo pipefail

zc_bin="$1"
source_root="$2"

python3 "$source_root/test/correctness/vector_add_host.py" \
  "$zc_bin" \
  "$source_root"
