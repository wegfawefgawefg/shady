fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let wave = 0.5 + 0.5 * sin(inputs.time + pixel.uv.x * 6.2831853);
    return vec4<f32>(pixel.uv.x, pixel.uv.y, wave, 1.0);
}
