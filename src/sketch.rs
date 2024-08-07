use glam::Vec2;
use raylib::prelude::*;

pub const FRAMES_PER_SECOND: u32 = 60;

pub struct State {
    pub running: bool,
    pub time_since_last_update: f32,
}

impl State {
    pub fn new() -> Self {
        Self {
            running: true,
            time_since_last_update: 0.0,
        }
    }
}

pub fn process_events_and_input(rl: &mut RaylibHandle, state: &mut State) {
    if rl.is_key_pressed(raylib::consts::KeyboardKey::KEY_ESCAPE) {
        state.running = false;
    }
}

pub fn step(rl: &mut RaylibHandle, rlt: &mut RaylibThread, state: &mut State) {}

pub fn draw(
    state: &State,
    d: &mut RaylibDrawHandle,
    shader: Option<&mut Shader>,
    shader_error: bool,
) {
    if shader_error {
        // Draw an error message or fallback rendering
        d.draw_text("Shader Error!", 12, 12, 24, Color::RED);
    } else if let Some(shader) = shader {
        // Get the screen dimensions
        let screen_width = d.get_screen_width() as f32;
        let screen_height = d.get_screen_height() as f32;

        // Set the screen dimensions uniform in the shader
        let screen_dims_loc = shader.get_shader_location("screenDims");
        shader.set_shader_value(screen_dims_loc, [screen_width, screen_height]);

        // passing a value to the shader
        // let uniform_loc = shader.get_shader_location("grayscaleFactor");
        // let t = dh.get_time() as f32;
        // let grayscale_factor = (t * 5.0).sin() * 0.5 + 0.5;
        // shader.set_shader_value(uniform_loc, grayscale_factor);

        d.clear_background(Color::WHITE);

        {
            let mut s = d.begin_shader_mode(shader);

            // Draw a rectangle covering the entire screen
            s.draw_rectangle(
                0,
                0,
                s.get_screen_width(),
                s.get_screen_height(),
                Color::BLACK,
            );

            // Draw any additional content here
            s.draw_text("Low Res Sketch!", 12, 12, 12, Color::BLACK);
        }
        let mouse_pos = d.get_mouse_position();
        d.draw_circle(mouse_pos.x as i32, mouse_pos.y as i32, 6.0, Color::RED);

        // draw a white rect at the bottom right corner
        d.draw_rectangle(
            d.get_screen_width() - 20,
            d.get_screen_height() - 20,
            20,
            20,
            Color::RED,
        );
    } else {
        // Fallback rendering without shader
        d.draw_text("No Shader Loaded", 12, 12, 24, Color::YELLOW);
    };
}
