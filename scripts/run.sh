#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
"${repo_root}/build-shady/shady" "${repo_root}/examples/pills/frosted_glass.wgsl" \
    --size 640x420 --scale 2
