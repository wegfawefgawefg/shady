#include "shady/arguments.hpp"
#include "shady/channels.hpp"
#include "shady/files.hpp"
#include "shady/gpu.hpp"
#include "shady/live_media.hpp"
#include "shady/presentation.hpp"

#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

namespace {

void wall_clock(shady::FrameState& frame) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    frame.year = local.tm_year + 1900;
    frame.month = local.tm_mon + 1;
    frame.day = local.tm_mday;
    frame.wall_clock_seconds = static_cast<float>(local.tm_hour * 3600 + local.tm_min * 60 +
                                                   local.tm_sec);
}

void pump_events(bool& running, bool& capture, shady::InputState& input, int height, int scale) {
    input.mouse_wheel_x = 0.0F;
    input.mouse_wheel_y = 0.0F;
    SDL_Event event{};
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            running = false;
        if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_F12)
            capture = true;
        if (event.type == SDL_MOUSEWHEEL) {
            input.mouse_wheel_x += static_cast<float>(event.wheel.x);
            input.mouse_wheel_y += static_cast<float>(event.wheel.y);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            input.mouse_click_x = static_cast<float>(event.button.x / scale);
            input.mouse_click_y = static_cast<float>(
                std::clamp(height - 1 - event.button.y / scale, 0, height - 1));
        }
    }

    int mouse_x = 0;
    int mouse_y = 0;
    const std::uint32_t buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    input.mouse_x = static_cast<float>(mouse_x / scale);
    input.mouse_y = static_cast<float>(std::clamp(height - 1 - mouse_y / scale, 0, height - 1));
    input.mouse_down = buttons != 0U ? 1.0F : 0.0F;
    for (int index = 0; index < shady::mouse_button_count; ++index) {
        input.mouse_buttons[static_cast<std::size_t>(index)] =
            (buttons & SDL_BUTTON(static_cast<unsigned int>(index + 1))) != 0U ? 1.0F : 0.0F;
    }
    int count = 0;
    const std::uint8_t* keys = SDL_GetKeyboardState(&count);
    const int copied = std::min(count, shady::key_count);
    for (int index = 0; index < copied; ++index)
        input.keys[static_cast<std::size_t>(index)] = keys[index] != 0U ? 1.0F : 0.0F;
}

void pump_gamepad(SDL_GameController* controller, shady::InputState& input) {
    if (controller == nullptr) return;
    for (int index = 0; index < SDL_CONTROLLER_BUTTON_MAX; ++index)
        input.gamepad_buttons[static_cast<std::size_t>(index)] =
            SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(index));
    constexpr float axis_scale = 1.0F / 32767.0F;
    for (int index = 0; index < SDL_CONTROLLER_AXIS_MAX; ++index)
        input.gamepad_axes[static_cast<std::size_t>(index)] =
            static_cast<float>(SDL_GameControllerGetAxis(
                controller, static_cast<SDL_GameControllerAxis>(index))) * axis_scale;
}

SDL_GameController* open_gamepad() {
    for (int index = 0; index < SDL_NumJoysticks(); ++index)
        if (SDL_IsGameController(index) == SDL_TRUE) return SDL_GameControllerOpen(index);
    return nullptr;
}

} // namespace

int main(int argc, char** argv) {
    const auto options = shady::parse_arguments(argc, argv, false);
    if (!options) {
        shady::print_usage(std::cerr, false);
        return 2;
    }
    if (options->show_help) {
        shady::print_usage(std::cout, false);
        return 0;
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0 ||
        !shady::init_channel_images()) {
        std::cerr << "could not initialize SDL\n";
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Shady", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, options->width * options->scale,
        options->height * options->scale, SDL_WINDOW_SHOWN);
    if (window == nullptr) return 1;

    // load project
    auto sources = shady::load_shader_sources(options->project, shady::default_runtime_paths());
    shady::ChannelSet channels;
    if (!sources || !shady::load_channels(*options, channels, options->time)) return 1;
    shady::LiveMedia live;
    if (!live.start(*options, channels)) return 1;
    shady::GpuContext gpu;
    shady::Renderer renderer;
    shady::Presentation presentation;
    if (!shady::make_gpu_context(gpu) ||
        !renderer.init(gpu, options->width, options->height, *sources, channels) ||
        !presentation.init(gpu, window, options->width, options->height, options->scale)) return 1;

    // run frames
    shady::SourceWatch watch(options->project);
    SDL_GameController* gamepad = open_gamepad();
    shady::FrameState frame;
    frame.width = options->width;
    frame.height = options->height;
    frame.frame = options->frame;
    frame.time = options->time;
    frame.time_delta = options->time_delta;
    const auto started = std::chrono::steady_clock::now();
    auto previous = started;
    bool running = true;
    int rendered = 0;
    while (running && (options->frames < 0 || rendered < options->frames)) {
        bool capture = false;
        pump_events(running, capture, frame.input, options->height, options->scale);
        pump_gamepad(gamepad, frame.input);
        if (!running) break;
        const auto now = std::chrono::steady_clock::now();
        frame.time_delta = std::chrono::duration<float>(now - previous).count();
        frame.time = options->time + std::chrono::duration<float>(now - started).count();
        previous = now;
        wall_clock(frame);
        if (!live.update(frame.time, channels)) break;
        for (std::size_t index = 0; index < channels.image.size(); ++index)
            if (!options->webcam_paths[index].empty() ||
                !options->microphone_paths[index].empty()) renderer.upload_channel(index, channels.image[index]);
        if (watch.changed()) {
            const auto changed = shady::load_shader_sources(options->project,
                                                            shady::default_runtime_paths());
            if (changed && renderer.reload(*changed)) {
                sources = changed;
                watch.acknowledge();
                std::cout << "reloaded\n";
            }
        }
        if (!renderer.render(frame, channels) || !presentation.present(renderer.output_view())) break;
        if (capture || (!options->output_path.empty() && options->frames > 0 &&
                        rendered + 1 == options->frames)) {
            const std::string path = options->output_path.empty() ? "shady-frame.png"
                                                                  : options->output_path;
            if (!shady::write_png(path, options->width, options->height, renderer.read_rgba()))
                std::cerr << "could not capture " << path << '\n';
        }
        gpu.device.poll(false);
        ++frame.frame;
        ++rendered;
    }

    if (gamepad != nullptr) SDL_GameControllerClose(gamepad);
    presentation.shutdown();
    SDL_DestroyWindow(window);
    shady::quit_channel_images();
    SDL_Quit();
    return 0;
}
