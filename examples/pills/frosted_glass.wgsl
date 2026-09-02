fn hash21(point: vec2<f32>) -> f32 {
    let p = fract(point * vec2<f32>(123.34, 345.45));
    return fract((p.x + p.y) * (p.x + p.y + 34.345));
}

fn rotate(point: vec2<f32>, angle: f32) -> vec2<f32> {
    let c = cos(angle);
    let s = sin(angle);
    return mat2x2<f32>(c, -s, s, c) * point;
}

fn capsule_distance(point: vec2<f32>, half_length: f32, radius: f32) -> f32 {
    let q = vec2<f32>(abs(point.x) - half_length, point.y);
    return length(vec2<f32>(max(q.x, 0.0), q.y)) + min(q.x, 0.0) - radius;
}

fn pill_distance(point: vec2<f32>, center: vec2<f32>, angle: f32, length: f32, radius: f32) -> f32 {
    return capsule_distance(rotate(point - center, -angle), length * 0.5 - radius, radius);
}

fn soft_pill(point: vec2<f32>, center: vec2<f32>, angle: f32, length: f32, radius: f32) -> f32 {
    let distance = pill_distance(point, center, angle, length, radius);
    return smoothstep(0.035, -0.012, distance);
}

fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let point = pixel.centered;
    let aspect_uv = pixel.coord / pixel.resolution;
    let grain = hash21(floor(pixel.coord * 0.45) + floor(inputs.time * 2.0));
    var glass = 0.0;
    var shadow = 0.0;

    let fall = fract(inputs.time * 0.11);
    let falling_center = vec2<f32>(0.48, 1.45 - fall * 2.8);
    glass = max(glass, soft_pill(point, falling_center, inputs.time * 0.7, 0.54, 0.075));
    shadow = max(shadow, soft_pill(point + vec2<f32>(0.018, -0.028), falling_center,
        inputs.time * 0.7, 0.56, 0.09));

    var centers = array<vec2<f32>, 7>(
        vec2<f32>(-0.72, -0.72), vec2<f32>(-0.34, -0.68), vec2<f32>(0.02, -0.76),
        vec2<f32>(0.38, -0.69), vec2<f32>(0.72, -0.74), vec2<f32>(-0.55, -0.43),
        vec2<f32>(0.53, -0.43),
    );
    var angles = array<f32, 7>(0.0, -0.58, -0.16, 0.92, 0.0, 0.94, -0.26);
    var lengths = array<f32, 7>(0.36, 0.42, 0.38, 0.43, 0.34, 0.39, 0.42);
    for (var index = 0; index < 7; index += 1) {
        let center = centers[index];
        glass = max(glass, soft_pill(point, center, angles[index], lengths[index], 0.065));
        shadow = max(shadow, soft_pill(point + vec2<f32>(0.018, -0.025), center,
            angles[index], lengths[index] + 0.025, 0.085));
    }

    var color = vec3<f32>(0.82, 0.85, 0.83);
    color += vec3<f32>(0.025) * (aspect_uv.y - 0.5);
    color -= vec3<f32>(0.29) * shadow * (1.0 - glass);
    let cloudy = 0.72 + grain * 0.08 + 0.08 * sin(point.x * 24.0 + point.y * 15.0);
    color = mix(color, vec3<f32>(cloudy), glass * 0.82);
    color -= vec3<f32>(0.42) * glass;
    color += vec3<f32>(0.06) * glass * smoothstep(-0.8, 0.8, -point.y);

    let floor_line = smoothstep(0.006, 0.0, abs(point.y + 0.86));
    color -= vec3<f32>(0.22) * floor_line;
    let vignette = smoothstep(1.35, 0.3, length(point * vec2<f32>(0.72, 1.0)));
    color *= 0.9 + 0.1 * vignette;
    return vec4<f32>(color, 1.0);
}
