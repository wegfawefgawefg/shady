#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace shady {

constexpr int channel_count = 4;
constexpr int key_count = 512;
constexpr int mouse_button_count = 8;
constexpr int gamepad_button_count = 32;
constexpr int gamepad_axis_count = 16;
constexpr int audio_texture_width = 512;
constexpr int audio_texture_height = 2;
constexpr int audio_sample_rate = 44100;

struct InputState {
    float mouse_x = 0.0F;
    float mouse_y = 0.0F;
    float mouse_down = 0.0F;
    float mouse_click_x = 0.0F;
    float mouse_click_y = 0.0F;
    float mouse_wheel_x = 0.0F;
    float mouse_wheel_y = 0.0F;
    std::array<float, key_count> keys{};
    std::array<float, mouse_button_count> mouse_buttons{};
    std::array<float, gamepad_button_count> gamepad_buttons{};
    std::array<float, gamepad_axis_count> gamepad_axes{};
};

struct FrameState {
    int width = 240;
    int height = 160;
    int frame = 0;
    float time = 0.0F;
    float time_delta = 1.0F / 60.0F;
    float wall_clock_seconds = 0.0F;
    int year = 1970;
    int month = 1;
    int day = 1;
    InputState input;
};

struct ImageChannel {
    int width = 0;
    int height = 0;
    float time = 0.0F;
    float sample_rate = 0.0F;
    std::vector<std::uint32_t> pixels;

    [[nodiscard]] bool loaded() const {
        return width > 0 && height > 0 && !pixels.empty();
    }
};

struct ChannelSet {
    std::array<ImageChannel, channel_count> image;
};

struct ShaderProject {
    std::string image_path;
    std::array<std::string, channel_count> buffer_paths;
};

struct ShaderSources {
    std::string image;
    std::array<std::string, channel_count> buffers;
    std::array<bool, channel_count> has_buffer{};
};

struct RuntimePaths {
    std::string prelude;
    std::string entry;
};

struct AppOptions {
    ShaderProject project;
    std::string output_path;
    int width = 640;
    int height = 420;
    int scale = 1;
    int frames = -1;
    int frame = 0;
    float time = 0.0F;
    float time_delta = 1.0F / 60.0F;
    bool headless = false;
    bool show_help = false;
    std::array<std::string, channel_count> image_paths;
    std::array<std::string, channel_count> noise_seeds;
    std::array<std::string, channel_count> video_paths;
    std::array<std::string, channel_count> webcam_paths;
    std::array<std::string, channel_count> audio_paths;
    std::array<std::string, channel_count> microphone_paths;
};

} // namespace shady
