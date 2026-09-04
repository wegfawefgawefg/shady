#!/usr/bin/env bash
set -euo pipefail

url="${1:-http://localhost:5173/}"
browser="${SHADY_BROWSER:-google-chrome}"
profile="${SHADY_BROWSER_PROFILE:-$(mktemp -d /tmp/shady-webgpu-profile.XXXXXX)}"

exec "$browser" \
  --user-data-dir="$profile" \
  --no-first-run \
  --no-default-browser-check \
  --enable-unsafe-webgpu \
  --enable-features=Vulkan \
  --ignore-gpu-blocklist \
  --new-window \
  "$url"
