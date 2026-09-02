fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let previous = channel_sample(0, pixel.uv).rgb * 0.975;
    let center = vec2<f32>(0.5) + vec2<f32>(sin(inputs.time * 1.3), cos(inputs.time)) * 0.28;
    let dot = smoothstep(0.035, 0.0, distance(pixel.uv, center));
    return vec4<f32>(max(previous, vec3<f32>(dot, dot * 0.45, dot * 0.12)), 1.0);
}
