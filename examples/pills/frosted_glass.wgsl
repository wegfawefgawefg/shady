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

fn projected_capsule_distance(
    point: vec2<f32>, half_segment: f32, near_radius: f32, far_radius: f32,
) -> f32 {
    let along = clamp(point.x, -half_segment, half_segment);
    let depth = (along + half_segment) / (2.0 * half_segment);
    let radius = mix(near_radius, far_radius, depth);
    return length(point - vec2<f32>(along, 0.0)) - radius;
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

    // Project a long pill tilted 45 degrees away from the glass.  Its far end
    // points along +x: it is smaller, deeper in the glaze, and less distinct.
    let tilt_projection = 0.7071;
    let half_segment = 0.29 * tilt_projection;
    let along = clamp(local.x, -half_segment, half_segment);
    let depth = (along + half_segment) / (2.0 * half_segment);
    let distance = projected_capsule_distance(local, half_segment, 0.115, 0.082);
    let haze_local = local - vec2<f32>(mix(0.012, 0.050, depth), -0.030);
    let haze_distance = projected_capsule_distance(
        haze_local, half_segment + 0.018, 0.135, 0.145);

    // The object is below the glass: broad diffusion first, then a softened
    // silhouette.  Every layer absorbs light, so the edge never becomes a glow.
    let deep_haze = smoothstep(mix(0.135, 0.230, depth), -0.045, haze_distance);
    let near_haze = smoothstep(mix(0.065, 0.125, depth), -0.035, distance);
    let body = smoothstep(mix(0.012, 0.032, depth), mix(-0.025, -0.016, depth), distance);
    let visibility = mix(1.08, 0.58, depth);

    var color = background(pixel.centered, pixel.coord);
    let cloudy = value_noise(local * vec2<f32>(7.0, 13.0) + vec2<f32>(inputs.time * 0.04, 0.0));
    color -= vec3<f32>(0.075, 0.077, 0.074) * deep_haze;
    color -= vec3<f32>(0.115, 0.120, 0.116) * near_haze * visibility;
    color -= vec3<f32>(0.390, 0.398, 0.385) * body * visibility;

    // Uneven milkiness makes the silhouette feel embedded in textured glass,
    // rather than like a crisp shape with a blur filter around it.
    color += vec3<f32>(0.018) * (cloudy - 0.5) * near_haze;

    let vignette = smoothstep(1.45, 0.32, length(pixel.centered * vec2<f32>(0.7, 1.0)));
    color *= 0.92 + 0.08 * vignette;
    return vec4<f32>(color, 1.0);
}
