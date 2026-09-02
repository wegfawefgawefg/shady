fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let size = max(channel_metadata(0).xy, vec2<f32>(1.0));
    let texel = 1.0 / size;
    let center = channel_sample(0, pixel.uv).rgb;
    let right = channel_sample(0, pixel.uv + vec2<f32>(texel.x, 0.0)).rgb;
    let down = channel_sample(0, pixel.uv + vec2<f32>(0.0, texel.y)).rgb;
    let edge = length(center - right) + length(center - down);
    return vec4<f32>(vec3<f32>(smoothstep(0.05, 0.35, edge)), 1.0);
}
