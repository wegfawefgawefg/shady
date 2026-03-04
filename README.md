# shady

Local fragment shader playground in Rust, powered by `raylib`.

This project is a fast iteration loop for shader experiments:
- Edit `src/shaders/fragment.fs`
- Save
- The app detects file changes and reloads the shader live

## Features

- Live shader hot-reload from disk
- Fullscreen shader pass over the window
- Useful uniforms wired in from Rust:
  - `screenDims` (`vec2`) window width/height in pixels
  - `mp` (`vec2`) mouse position normalized to `[0, 1]`
  - `time` (`float`) elapsed time in seconds
  - `metaballPositions` (`vec2[]`) debug metaball positions
  - `numMetaballs` (`int`) active metaball count
- Basic fallback/error messaging when shader compilation fails

## Project Layout

- `src/main.rs`: app setup, window loop, shader load + hot-reload
- `src/sketch.rs`: frame update/draw logic and uniform uploads
- `src/shaders/fragment.fs`: main fragment shader you edit
- `src/shaders/store.fs`: scratch/utility shader snippets

## Run

```bash
cargo run
```

Window title: `shady`

Default window size is currently `900x900` (see `src/main.rs`).

## Controls

- `Esc`: quit

## Shader Notes

Your fragment shader is loaded as GLSL `#version 330`.

Current flow in `fragment.fs`:
- Normalizes pixel coordinates to `uv`
- Draws a red circle at mouse position (`mp`)
- Draws tiny green debug markers at metaball positions

If the shader fails to compile, the app keeps running and shows an on-screen error indicator.

## Next Improvements (optional)

- Add keyboard toggles for debug overlays
- Send audio/reactive uniforms
- Add shader presets and quick switching
- Surface compile errors in-window with line hints
