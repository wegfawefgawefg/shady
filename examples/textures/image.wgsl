fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let drift = vec2<f32>(inputs.time * 0.03, 0.0);
    return channel_sample(0, fract(pixel.uv + drift));
}
