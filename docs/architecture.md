# Architecture

Shady has two shells and one shader contract. The native shell is C++20, SDL2, and
wgpu-native. The browser shell is TypeScript, CodeMirror, and the browser WebGPU API.
Both concatenate the same three pieces in the same order: runtime prelude, authored
WGSL, runtime entry. A shader that only uses the documented contract therefore has no
backend-specific branch.

## Native frame path

`app.cpp` is the conductor. It asks `arguments.cpp` for an `AppOptions`, asks
`files.cpp` for composed image and buffer sources, and asks `channels.cpp` for initial
textures. `live_media.cpp` may replace some of those textures with webcam or microphone
frames. None of these modules knows how a compute pass is encoded.

`gpu.cpp` owns the WebGPU objects that survive between frames. It creates one uniform
buffer, four input textures, two textures for each possible feedback slot, and one final
output texture. Feedback textures exist even when their pass is inactive; this matters
because hot reload can introduce a buffer without tearing down the window or losing all
other GPU state.

At the start of a frame, every active buffer pass sees the same previous-frame channel
set. The passes do not see one another's half-written current results. After all buffer
passes finish, the image pass sees every current buffer result plus the ordinary input
textures in unused slots. The read and write sides swap only after the image pass. This
makes the result independent of buffer iteration order and matches the useful part of
Shadertoy's feedback model.

`presentation.cpp` is the final bridge. It samples the output texture with `textureLoad`
and draws one full-screen triangle into the SDL window's WebGPU surface. Integer scale
is deliberate: retro-sized projects stay sharp and there is no sampler object hiding
filter behavior. `shady-frame` skips this module and copies the same output texture into
a mapped buffer for a deterministic PPM file.

## Browser frame path

The browser stores a project as files plus settings. `project.ts` validates and encodes
that model. `shader.ts` performs source composition. `webgpu.ts` owns textures,
pipelines, feedback, media uploads, and frame dispatch. `main.ts` conducts the editor,
controls, URL sharing, PNG export, and recording. This is intentionally the same shape
as the native side even though DOM work makes the browser conductor larger.

Projects are compressed into the location hash for sharing. Nothing is uploaded to a
server. Image, video, and audio inputs may be embedded as data URLs, while webcam and
microphone streams remain local and are requested through browser permissions.

## Failure behavior

Initial shader failure is fatal because there is no meaningful image to show. A reload
is different. `Renderer::reload` creates a complete candidate image pipeline and all
candidate buffer pipelines before replacing any active pipeline. If any candidate
fails, the method returns without mutating the live set. The watcher is not acknowledged,
so another save retries the edit. This is the last-good behavior that makes shader
iteration pleasant instead of fragile.

GPU validation belongs to WebGPU. Shady does only the one structural check that source
composition needs: the authored file must contain `fn shade`. It should not grow a
parallel WGSL parser. Better diagnostics and source maps are useful future work, but a
second shader language is specifically outside the product boundary.
