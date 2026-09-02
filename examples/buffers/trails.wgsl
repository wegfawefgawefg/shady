fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let trail = channel_sample(0, pixel.uv).rgb;
    return vec4<f32>(pow(trail, vec3<f32>(0.75)), 1.0);
}
