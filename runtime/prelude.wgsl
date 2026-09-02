struct ShadyInputs {
    time: f32,
    time_delta: f32,
    frame: f32,
    width: f32,
    height: f32,
    mouse_x: f32,
    mouse_y: f32,
    mouse_down: f32,
    mouse_click_x: f32,
    mouse_click_y: f32,
    wall_clock_seconds: f32,
    year: f32,
    month: f32,
    day: f32,
    channel0: vec4<f32>,
    channel1: vec4<f32>,
    channel2: vec4<f32>,
    channel3: vec4<f32>,
    keys: array<vec4<f32>, 128>,
    mouse_buttons: array<vec4<f32>, 2>,
    mouse_wheel: vec4<f32>,
    gamepad_buttons: array<vec4<f32>, 8>,
    gamepad_axes: array<vec4<f32>, 4>,
};

struct ShadyPixel {
    coord: vec2<f32>,
    uv: vec2<f32>,
    centered: vec2<f32>,
    resolution: vec2<f32>,
};

@group(0) @binding(0) var shady_output: texture_storage_2d<rgba8unorm, write>;
@group(0) @binding(1) var<uniform> inputs: ShadyInputs;
@group(0) @binding(2) var channel0: texture_2d<f32>;
@group(0) @binding(3) var channel1: texture_2d<f32>;
@group(0) @binding(4) var channel2: texture_2d<f32>;
@group(0) @binding(5) var channel3: texture_2d<f32>;

fn channel_metadata(index: i32) -> vec4<f32> {
    switch index {
        case 0: { return inputs.channel0; }
        case 1: { return inputs.channel1; }
        case 2: { return inputs.channel2; }
        case 3: { return inputs.channel3; }
        default: { return vec4<f32>(0.0); }
    }
}

fn channel_load(index: i32, coord: vec2<i32>) -> vec4<f32> {
    let size = max(vec2<i32>(channel_metadata(index).xy), vec2<i32>(1));
    let at = clamp(coord, vec2<i32>(0), size - vec2<i32>(1));
    switch index {
        case 0: { return textureLoad(channel0, at, 0); }
        case 1: { return textureLoad(channel1, at, 0); }
        case 2: { return textureLoad(channel2, at, 0); }
        case 3: { return textureLoad(channel3, at, 0); }
        default: { return vec4<f32>(0.0); }
    }
}

fn channel_sample(index: i32, uv: vec2<f32>) -> vec4<f32> {
    let size = max(channel_metadata(index).xy, vec2<f32>(1.0));
    let coord = vec2<i32>(floor(clamp(uv, vec2<f32>(0.0), vec2<f32>(0.999999)) * size));
    return channel_load(index, coord);
}

fn packed_read(value: vec4<f32>, lane: i32) -> f32 {
    switch lane {
        case 0: { return value.x; }
        case 1: { return value.y; }
        case 2: { return value.z; }
        case 3: { return value.w; }
        default: { return 0.0; }
    }
}

fn key_down(scancode: i32) -> f32 {
    if scancode < 0 || scancode >= 512 {
        return 0.0;
    }
    return packed_read(inputs.keys[scancode / 4], scancode % 4);
}

fn mouse_button_down(button: i32) -> f32 {
    if button < 0 || button >= 8 {
        return 0.0;
    }
    return packed_read(inputs.mouse_buttons[button / 4], button % 4);
}

fn gamepad_button_down(button: i32) -> f32 {
    if button < 0 || button >= 32 {
        return 0.0;
    }
    return packed_read(inputs.gamepad_buttons[button / 4], button % 4);
}

fn gamepad_axis(axis: i32) -> f32 {
    if axis < 0 || axis >= 16 {
        return 0.0;
    }
    return packed_read(inputs.gamepad_axes[axis / 4], axis % 4);
}
