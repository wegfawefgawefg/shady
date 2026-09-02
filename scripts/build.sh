#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
cd "${repo_root}"
cmake --preset dev
cmake --build --preset dev
ctest --test-dir build-shady --output-on-failure
