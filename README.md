# Shady

Shady is a small Shadertoy-shaped WGSL workshop. It gives one shader the screen, a
clock, mouse and keyboard state, four texture channels, four optional feedback passes,
hot reload, and almost nothing else. The same project model runs as a native C++20
program on wgpu-native/Vulkan and as a browser editor on WebGPU.

The default sketch is the frosted pill study in
[`examples/pills/frosted_glass.wgsl`](examples/pills/frosted_glass.wgsl). It is the
reason this repository exists: a place to reproduce a visual quickly, tune it live,
and keep the shader portable enough to rewrite for Shadertoy later.

## Build and run

On Ubuntu, install CMake, a C++20 compiler, SDL2, SDL2_image, pkg-config, and optionally
FFmpeg for video, audio, webcam, and microphone channels. CMake downloads the pinned
WebGPU-distribution package and uses its wgpu-native backend.

```sh
sudo apt install cmake g++ pkg-config libsdl2-dev libsdl2-image-dev ffmpeg
./scripts/build.sh
./build-shady/shady examples/pills/frosted_glass.wgsl --size 640x420 --scale 2
```

The native window currently creates its WebGPU surface through SDL/X11. Press F12 to
write `shady-frame.png`, Escape to quit, and save any project shader to hot reload it.
An invalid edit is reported by WebGPU while the last good pipeline remains active.

For deterministic automation, render one frame without a window:

```sh
./build-shady/shady-frame examples/pills/frosted_glass.wgsl \
  --size 640x420 --time 1.25 --output /tmp/pills.ppm
```

Run the browser workshop independently:

```sh
cd web
npm install
npm test
npm run dev
```

## The shader contract

A project file is ordinary WGSL containing one function:

```wgsl
fn shade(pixel: ShadyPixel) -> vec4<f32> {
    return vec4<f32>(pixel.uv, 0.5 + 0.5 * sin(inputs.time), 1.0);
}
```

Shady prepends [`runtime/prelude.wgsl`](runtime/prelude.wgsl) and appends
[`runtime/entry.wgsl`](runtime/entry.wgsl). This deliberately small source composition
step supplies the compute entry point and stable bindings; there is no custom language,
assembler, transpiler, or hidden shader compiler. Coordinates use a bottom-left origin
like Shadertoy. The authored file can use `inputs`, `channel_sample`, `channel_load`,
`key_down`, `mouse_button_down`, `gamepad_button_down`, and `gamepad_axis` directly.

Pass `--channel0 image.png`, `--noise1 seed`, `--video2 clip.mp4`, or `--audio3 song.wav`
for static channels. The desktop runner also accepts `--webcam0 /dev/video0` and
`--mic1 default`. A feedback project supplies a shader with `--buffer0 buffer.wgsl`;
buffer passes read the previous frame and the image pass reads the newly written frame.

## Repository map

The native program is intentionally boring C+. `arguments` owns command-line meaning,
`channels` and `media` own pixels, `files` owns source composition and watching, `gpu`
owns compute dispatch and feedback, `presentation` owns the SDL WebGPU surface,
`live_media` owns streaming devices, and `app` only conducts the loop. The browser has
the same conceptual split under `web/src`.

The design and completion contract live in [`docs/goal.md`](docs/goal.md). Read
[`docs/architecture.md`](docs/architecture.md) for the exact frame path,
[`docs/shader-api.md`](docs/shader-api.md) for every shader-visible value, and
[`docs/projects.md`](docs/projects.md) for channels, buffers, sharing, and export.

Shady is descended from `asm-shader-toy`; its native and browser harnesses were the
useful prototype. Shady removes the fake assembly language and keeps direct WGSL as the
single authored format. It is MIT licensed.
