fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let cells = floor(pixel.uv * 80.0) / 80.0;
    let noise = channel_sample(0, fract(cells + vec2<f32>(inputs.time * 0.04, 0.0))).rgb;
    return vec4<f32>(noise, 1.0);
}
