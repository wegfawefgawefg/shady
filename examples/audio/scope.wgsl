fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let waveform = channel_sample(0, vec2<f32>(pixel.uv.x, 0.25)).r;
    let spectrum = channel_sample(0, vec2<f32>(pixel.uv.x, 0.75)).r;
    let wave_line = smoothstep(0.018, 0.0, abs(pixel.uv.y - waveform));
    let bars = step(pixel.uv.y, spectrum * 0.8);
    return vec4<f32>(0.08 + bars * 0.2, 0.1 + wave_line * 0.9, 0.14 + bars * 0.8, 1.0);
}
