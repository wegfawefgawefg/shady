fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let mouse = vec2<f32>(inputs.mouse_x, inputs.mouse_y) / pixel.resolution;
    let mouse_ring = smoothstep(0.035, 0.025, abs(distance(pixel.uv, mouse) - 0.08));
    let keyboard = max(key_down(44), key_down(26));
    let wheel = clamp(abs(inputs.mouse_wheel.y), 0.0, 1.0);
    let gamepad = clamp(abs(gamepad_axis(0)), 0.0, 1.0);
    let base = vec3<f32>(pixel.uv, 0.2 + 0.3 * sin(inputs.time));
    let color = base + vec3<f32>(mouse_ring, keyboard, max(wheel, gamepad));
    return vec4<f32>(color, 1.0);
}
