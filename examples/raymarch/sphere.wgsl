fn sphere_distance(point: vec3<f32>, radius: f32) -> f32 {
    return length(point) - radius;
}

fn scene_distance(point: vec3<f32>) -> f32 {
    let sphere = sphere_distance(point, 0.72);
    let floor = point.y + 0.9;
    return min(sphere, floor);
}

fn scene_normal(point: vec3<f32>) -> vec3<f32> {
    let e = 0.001;
    let d = scene_distance(point);
    return normalize(vec3<f32>(
        scene_distance(point + vec3<f32>(e, 0.0, 0.0)) - d,
        scene_distance(point + vec3<f32>(0.0, e, 0.0)) - d,
        scene_distance(point + vec3<f32>(0.0, 0.0, e)) - d,
    ));
}

fn shade(pixel: ShadyPixel) -> vec4<f32> {
    let origin = vec3<f32>(0.0, 0.0, -3.0);
    let direction = normalize(vec3<f32>(pixel.centered, 1.8));
    var travel = 0.0;
    var hit = false;
    for (var step = 0; step < 96; step += 1) {
        let distance = scene_distance(origin + direction * travel);
        if distance < 0.001 {
            hit = true;
            break;
        }
        travel += distance;
        if travel > 8.0 { break; }
    }
    if !hit {
        return vec4<f32>(0.75, 0.81, 0.84, 1.0);
    }
    let point = origin + direction * travel;
    let normal = scene_normal(point);
    let light = normalize(vec3<f32>(-0.5, 0.8, -0.6));
    let diffuse = 0.18 + 0.82 * max(dot(normal, light), 0.0);
    return vec4<f32>(vec3<f32>(diffuse), 1.0);
}
