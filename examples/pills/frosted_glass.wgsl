fn hash21(point: vec2<f32>) -> f32 {
    let p = fract(point * vec2<f32>(123.34, 345.45));
    return fract((p.x + p.y) * (p.x + p.y + 34.345));
}

fn value_noise(point: vec2<f32>) -> f32 {
    let cell = floor(point);
    let blend = fract(point) * fract(point) * (3.0 - 2.0 * fract(point));
    let low = mix(hash21(cell), hash21(cell + vec2<f32>(1.0, 0.0)), blend.x);
    let high = mix(hash21(cell + vec2<f32>(0.0, 1.0)),
        hash21(cell + vec2<f32>(1.0, 1.0)), blend.x);
    return mix(low, high, blend.y);
}

fn rotate(point: vec2<f32>, angle: f32) -> vec2<f32> {
    let cosine = cos(angle);
    let sine = sin(angle);
    return vec2<f32>(
        cosine * point.x - sine * point.y,
        sine * point.x + cosine * point.y,
    );
}

fn capsule_distance(point: vec2<f32>, half_segment: f32, radius: f32) -> f32 {
    let nearest = vec2<f32>(clamp(point.x, -half_segment, half_segment), 0.0);
    return length(point - nearest) - radius;
}

fn mouse_point(value: vec2<f32>, resolution: vec2<f32>) -> vec2<f32> {
    return (value * 2.0 - resolution) / resolution.y;
}

fn background(point: vec2<f32>, coord: vec2<f32>) -> vec3<f32> {
    let broad = value_noise(point * 1.7 + vec2<f32>(0.0, inputs.time * 0.025));
    let grain = hash21(floor(coord * 0.42) + floor(inputs.time * 1.5));
    let vertical = 0.018 * point.y;
    return vec3<f32>(0.835, 0.855, 0.84) + vertical + 0.018 * broad + 0.009 * grain;
}

fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let cursor = mouse_point(vec2<f32>(inputs.mouse_x, inputs.mouse_y), pixel.resolution);
    let clicked = mouse_point(
        vec2<f32>(inputs.mouse_click_x, inputs.mouse_click_y), pixel.resolution);
    let has_click = inputs.mouse_click_x + inputs.mouse_click_y > 1.0;
    let center = select(vec2<f32>(0.0, -0.08), clicked, has_click);
    let drag = cursor - center;
    let has_direction = has_click && dot(drag, drag) > 0.0025;
    let angle = select(-0.28, atan2(drag.y, drag.x), has_direction);

    let local = rotate(pixel.centered - center, -angle);
    let distance = capsule_distance(local, 0.245, 0.105);
    let shadow_local = rotate(pixel.centered - center - vec2<f32>(0.025, -0.045), -angle);
    let shadow_distance = capsule_distance(shadow_local, 0.26, 0.13);

    let soft_shadow = smoothstep(0.16, -0.035, shadow_distance);
    let outer_frost = smoothstep(0.105, -0.018, distance);
    let glass = smoothstep(0.025, -0.018, distance);
    let dense_core = smoothstep(-0.005, -0.085, distance);
    let inner_edge = smoothstep(0.018, -0.018, distance) -
        smoothstep(-0.028, -0.082, distance);

    let edge_direction = normalize(vec2<f32>(local.x * 0.35, local.y) + vec2<f32>(0.0001));
    let refracted_point = pixel.centered + edge_direction * glass * 0.045;
    var color = background(pixel.centered, pixel.coord);
    color -= vec3<f32>(0.19) * soft_shadow * (1.0 - glass * 0.72);

    let refracted = background(refracted_point, pixel.coord + edge_direction * 9.0);
    let cloudy = value_noise(local * vec2<f32>(7.0, 13.0) + vec2<f32>(inputs.time * 0.04, 0.0));
    let frosted = refracted * (0.72 + cloudy * 0.13) - vec3<f32>(0.13);
    color = mix(color, frosted, glass * 0.82);
    color -= vec3<f32>(0.23) * dense_core;
    color += vec3<f32>(0.105, 0.115, 0.108) * inner_edge *
        smoothstep(-0.75, 0.65, edge_direction.y);
    color = mix(color, color * vec3<f32>(0.86, 0.89, 0.87), outer_frost * 0.18);

    let vignette = smoothstep(1.45, 0.32, length(pixel.centered * vec2<f32>(0.7, 1.0)));
    color *= 0.92 + 0.08 * vignette;
    return vec4<f32>(color, 1.0);
}
