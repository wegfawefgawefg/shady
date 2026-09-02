#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
frame="${repo_root}/build-shady/shady-frame"
output="/tmp/shady-example-validation.ppm"
media="${1:-all}"

for shader in \
    examples/basics/gradient.wgsl \
    examples/basics/plasma.wgsl \
    examples/input/live_controls.wgsl \
    examples/raymarch/sphere.wgsl \
    examples/pills/frosted_glass.wgsl; do
    "${frame}" "${repo_root}/${shader}" --size 64x40 --time 0.5 --output "${output}"
done

"${frame}" "${repo_root}/examples/textures/noise.wgsl" \
    --noise0 validation --size 64x40 --output "${output}"
"${frame}" "${repo_root}/examples/textures/image.wgsl" \
    --channel0 "${repo_root}/examples/assets/checker.png" --size 64x40 --output "${output}"
if [ "${media}" != "--skip-media" ]; then
    "${frame}" "${repo_root}/examples/audio/scope.wgsl" \
        --audio0 "${repo_root}/examples/assets/audio/two_tone.wav" --size 64x40 --output "${output}"
    "${frame}" "${repo_root}/examples/video/edges.wgsl" \
        --video0 "${repo_root}/examples/assets/video/testsrc_160x90.mp4" \
        --time 0.5 --size 64x40 --output "${output}"
fi
"${frame}" "${repo_root}/examples/buffers/trails.wgsl" \
    --buffer0 "${repo_root}/examples/buffers/trails_buffer.wgsl" \
    --size 64x40 --output "${output}"

echo "all WGSL examples validated"
