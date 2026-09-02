# Projects and tools

The native project model is a main WGSL path plus zero to four buffer WGSL paths. Other
command-line options supply channels and timing. The browser serializes the same idea as
JSON because its files may exist only in the editor. Export from the browser produces
that JSON bundle; import restores it; Share compresses it into the URL fragment.

Image and generated-noise channels are timeless. Video and audio files loop at the
requested time and depend on `ffprobe` and `ffmpeg` in the native runner. Webcam capture
uses Linux v4l2 and microphone capture uses PulseAudio through FFmpeg. The browser uses
`getUserMedia`, `HTMLVideoElement`, and the Web Audio API instead. A missing media tool
is reported as a channel load failure rather than replaced with surprising pixels.

Feedback is explicit. `--buffer0 examples/buffers/trails_buffer.wgsl` assigns that pass
to channel zero, and the image shader reads its current result from channel zero. The
buffer itself reads channel zero from the previous frame. The browser exposes the same
assignment in its Buffer controls. All passes use the same resolution and shader API.

The desktop runner follows wall time by default. `--time` adds an initial offset and
`--frames` gives it a finite frame count, which is convenient for smoke tests and final
capture. `shady-frame` is fully deterministic: it uses the exact `--time`, `--frame`,
and `--time-delta` values and never opens a window.

The repository scripts keep the common path short. `scripts/build.sh` configures the
strict debug preset, builds every native target, and runs CTest. `scripts/run.sh` opens
the pill sketch. Browser tests and production compilation are ordinary npm scripts in
`web/package.json`. GitHub Pages runs those tests and deploys `web/dist`.

Shady intentionally does not pretend to be source-compatible with Shadertoy. WGSL and
GLSL differ in types, texture calls, matrices, and entry syntax. The portable part is
the workshop: one image function, time and interaction, four channels, feedback, and a
quick way to share the result. Once the composition looks right, a focused WGSL-to-GLSL
rewrite is much smaller than recreating the interface around every experiment.
