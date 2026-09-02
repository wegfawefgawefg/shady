#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ ! -x "${repo_root}/build-shady/shady" ]; then
    "${repo_root}/scripts/build.sh"
fi

exec "${repo_root}/build-shady/shady" \
    "${repo_root}/examples/pills/frosted_glass.wgsl" \
    --size 640x420 --scale 1 "$@"
