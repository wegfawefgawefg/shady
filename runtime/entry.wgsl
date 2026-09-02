@compute @workgroup_size(8, 8, 1)
fn main(@builtin(global_invocation_id) gid: vec3<u32>) {
    if gid.x >= u32(inputs.width) || gid.y >= u32(inputs.height) {
        return;
    }

    let resolution = vec2<f32>(inputs.width, inputs.height);
    let coord = vec2<f32>(f32(gid.x) + 0.5, inputs.height - f32(gid.y) - 0.5);
    let uv = coord / resolution;
    let centered = (coord * 2.0 - resolution) / resolution.y;
    let pixel = ShadyPixel(coord, uv, centered, resolution);
    textureStore(shady_output, vec2<i32>(gid.xy), shade(pixel));
}
