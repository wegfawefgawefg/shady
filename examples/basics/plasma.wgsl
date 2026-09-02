fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let p = pixel.centered;
    let a = sin(p.x * 5.0 + inputs.time);
    let b = cos(p.y * 7.0 - inputs.time * 0.7);
    let c = sin(length(p) * 9.0 - inputs.time * 1.4);
    let value = 0.5 + 0.5 * sin((a + b + c) * 2.2);
    let color = 0.5 + 0.5 * cos(vec3<f32>(0.0, 2.0, 4.0) + value * 5.0 + inputs.time * 0.2);
    return vec4<f32>(color, 1.0);
}
