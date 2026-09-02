# Shader API

Every authored image and buffer file defines `fn shade(pixel: ShadyPixel) -> vec4<f32>`.
Shady invokes it once per output pixel from an 8 by 8 compute grid and writes the result
to an `rgba8unorm` texture. Color values outside zero through one are clamped by that
texture format.

`pixel.coord` is the pixel center in project pixels with a bottom-left origin.
`pixel.uv` is that coordinate divided by resolution. `pixel.centered` has zero at the
middle, one unit equal to the image height, and preserves aspect ratio. `pixel.resolution`
is the project width and height. This makes small Shadertoy ports mostly a matter of
renaming the entry function and replacing `iResolution` and `iTime`.

`inputs.time`, `inputs.time_delta`, and `inputs.frame` describe animation. Width and
height repeat the resolution for code that wants only the uniform. Mouse coordinates and
click coordinates use the same bottom-left pixel space. `inputs.mouse_down` is one when
any mouse button is held. The wall clock fields provide seconds since local midnight,
year, month, and day.

Each `inputs.channelN` is a `vec4` containing width, height, playback time, and sample
rate. `channel_load(index, coordinate)` reads an integer texel with edge clamping.
`channel_sample(index, uv)` converts normalized coordinates to a nearest texel. Shady
does not create a sampler, so authored code never silently changes from nearest to
linear filtering between backends. Write filtering explicitly when an effect needs it.

Keyboard state is indexed by SDL-compatible scancode in `key_down`. Mouse buttons,
gamepad buttons, and gamepad axes use their corresponding integer slots. All helpers
return floats so they mix directly into shader math. Unsupported or out-of-range slots
return zero. The browser maps its available key codes into the same 512-slot packed
uniform and polls the first connected gamepad.

Audio textures are 512 by 2. Row zero is the recent mono waveform remapped from minus
one through one into zero through one. Row one is a compact spectrum. The channel sample
rate is 44100 for files and live microphone input. Video and webcam channels are RGBA8;
the webcam is mirrored before upload so it behaves like a familiar preview.

The binding declarations themselves are public in `runtime/prelude.wgsl`, but authored
files should use the named API rather than redeclaring group zero. Group zero bindings
zero through five belong to Shady. Additional bind groups are not currently exposed by
the project model.
