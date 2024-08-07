use glam::{IVec2, UVec2};
use raylib::prelude::*;
use raylib::{ffi::SetTraceLogLevel, prelude::TraceLogLevel};
use std::fs;
use std::time::SystemTime;

mod sketch;

const TIMESTEP: f32 = 1.0 / sketch::FRAMES_PER_SECOND as f32;
const SHADER_PATH: &str = "src/shaders/fragment.fs";

fn main() {
    let mut state = sketch::State::new();
    let (mut rl, mut rlt) = raylib::init().title("shady").build();
    unsafe {
        SetTraceLogLevel(TraceLogLevel::LOG_WARNING as i32);
    }
    // let window_dims = UVec2::new(1280, 720);
    let window_dims = UVec2::new(500, 500);
    let fullscreen = false;
    rl.set_window_size(window_dims.x as i32, window_dims.y as i32);
    if fullscreen {
        rl.toggle_fullscreen();
        rl.set_window_size(rl.get_screen_width(), rl.get_screen_height());
    }
    center_window(&mut rl, window_dims);

    let mut shader = load_shader(&mut rl, &rlt);
    let mut last_modified = fs::metadata(SHADER_PATH).and_then(|m| m.modified()).ok();
    let mut shader_error = shader.is_none();

    while state.running && !rl.window_should_close() {
        // Check if shader file has been modified
        if let Ok(modified) = fs::metadata(SHADER_PATH).and_then(|m| m.modified()) {
            if last_modified.map_or(true, |last| modified > last) {
                shader = load_shader(&mut rl, &rlt);
                shader_error = shader.is_none();
                last_modified = Some(modified);
            }
        }

        sketch::process_events_and_input(&mut rl, &mut state);
        let dt = rl.get_frame_time();
        state.time_since_last_update += dt;
        while state.time_since_last_update > TIMESTEP {
            state.time_since_last_update -= TIMESTEP;
            sketch::step(&mut rl, &mut rlt, &mut state);
        }

        let mut draw_handle = rl.begin_drawing(&rlt);
        {
            draw_handle.clear_background(Color::BLACK);
            sketch::draw(&state, &mut draw_handle, shader.as_mut(), shader_error);
        }
    }
}

fn load_shader(rl: &mut RaylibHandle, rlt: &RaylibThread) -> Option<Shader> {
    match rl.load_shader(rlt, None, Some(SHADER_PATH)) {
        Ok(shader) => Some(shader),
        Err(e) => {
            println!("Error loading shader: {}", e);
            None
        }
    }
}

pub fn center_window(rl: &mut raylib::RaylibHandle, window_dims: UVec2) {
    let screen_dims = IVec2::new(rl.get_screen_width(), rl.get_screen_height());
    let screen_center = screen_dims / 2;
    let window_center = window_dims.as_ivec2() / 2;
    let offset = IVec2::new(screen_center.x, screen_center.y + window_center.y);
    rl.set_window_position(offset.x, offset.y);
    rl.set_target_fps(144);
}
