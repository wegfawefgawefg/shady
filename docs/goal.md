# Shady Goal

Shady is a local shader playground for writing WGSL without building a renderer around
every experiment. It replaces `local-c-shadertoy` and keeps the useful rendering work
from `asm-shader-toy` while removing that project's assembly language, assembler,
intermediate representation, virtual machine, and source-to-WGSL compiler.

The primary program is a C++20 desktop application. It uses SDL for windows and input,
WebGPU for rendering, and WGSL as the only authored shader language. On Linux the WGPU
implementation normally selects Vulkan. The program does not expose Vulkan directly.
The browser application uses the same WGSL project files and the browser WebGPU API.

## Product boundary

The finished repository includes all of the following:

- a native windowed runner with hot reload and last-good-pipeline behavior;
- a native headless renderer for deterministic frames, screenshots, and CI;
- a browser editor with live compilation, project import/export, share URLs, frame
  export, clipboard copy, and video recording;
- image, generated-noise, video, webcam, audio, and microphone channels;
- four feedback buffer passes with explicit previous/current frame behavior;
- resolution, time, delta, frame, date, mouse, keyboard, mouse button, mouse wheel, and
  gamepad inputs;
- checked-in examples for basic color, input, textures, feedback, audio, video,
  raymarching, and the frosted pill study that motivated the project;
- native and browser tests, validation scripts, build presets, and CI;
- prose documentation for the runtime contract, project format, architecture,
  controls, media behavior, and implementation decisions.

Shady is not a general game engine, a node graph, a GLSL compatibility layer, or an
assembly-language environment. Shader code is WGSL. Supporting another authored shader
language requires a separate proposal because it creates a second compiler and runtime
contract.

## Architecture target

The native application is divided by ownership rather than by vague utility layers:

- `app` owns arguments, clocks, reload decisions, and the main loop;
- `gpu` owns the WebGPU instance, adapter, device, queues, pipelines, and readback;
- `surface` owns SDL presentation and swapchain configuration;
- `project` owns the image pass, buffer passes, dimensions, and serialized settings;
- `channels` owns static images and generated noise;
- `media` owns ffmpeg-backed video, webcam, audio, and microphone streams;
- `inputs` owns mouse, keyboard, wheel, gamepad, date, and uniform packing;
- `watch` owns file timestamps and last-good source state;
- `capture` owns PNG output and deterministic headless frames.

The browser follows the same boundaries where browser APIs differ. Shared behavior is
defined by the documented bind layout and project JSON format, not by attempting to
share C++ and TypeScript implementation code.

Implementation files target roughly 300 to 500 lines. Files may be shorter when they
own one small concept. Files larger than 500 lines are split by concrete responsibility.
Comments state what a block does in terse phrases. Rationale and operating details live
in `docs/`.

## Work sequence

1. Preserve the upstream history and rename the inherited remote so accidental pushes
   cannot modify `asm-shader-toy`.
2. Record the direct-WGSL runtime contract and project format.
3. Replace assembly examples with WGSL image and buffer passes.
4. Remove the assembler, IR, VM, CPU renderer, source compiler, standard library, and
   their tests.
5. Split the native WebGPU frame and window runners into the ownership boundaries above.
6. Make direct WGSL loading, validation, hot reload, presentation, headless rendering,
   channels, media, feedback, and capture work together.
7. Replace the browser assembly editor and compiler with a direct WGSL project editor.
8. Preserve browser channels, live media, feedback, export, recording, and share links.
9. Replace ASM-specific scripts, tests, CI, names, screenshots, and documentation.
10. Build and test native CPU-independent components, native WebGPU rendering, browser
    unit tests, browser production output, and checked-in examples.
11. Audit source sizes, stale ASM terminology, repository status, and documentation.

## Completion test

The conversion is complete when a fresh checkout can build the native tools and browser
application using documented commands; every checked-in WGSL example validates; native
headless rendering produces deterministic images; the native window and browser render
the same project contract; media and feedback paths are exercised; no runtime target
links the old language implementation; and repository-wide searches find no surviving
ASM product behavior outside historical attribution.
