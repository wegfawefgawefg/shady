#include "shady/arguments.hpp"
#include "shady/channels.hpp"
#include "shady/files.hpp"
#include "shady/gpu.hpp"

#include <chrono>
#include <ctime>
#include <iostream>

namespace {

shady::FrameState make_frame(const shady::AppOptions& options) {
    shady::FrameState frame;
    frame.width = options.width;
    frame.height = options.height;
    frame.frame = options.frame;
    frame.time = options.time;
    frame.time_delta = options.time_delta;

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
    return frame;
}

} // namespace

int main(int argc, char** argv) {
    const auto options = shady::parse_arguments(argc, argv, true);
    if (!options) {
        shady::print_usage(std::cerr, true);
        return 2;
    }
    if (options->show_help) {
        shady::print_usage(std::cout, true);
        return 0;
    }

    // load project
    const auto sources = shady::load_shader_sources(options->project,
                                                     shady::default_runtime_paths());
    if (!sources) {
        std::cerr << "could not load shader project\n";
        return 1;
    }
    if (!shady::init_channel_images()) {
        std::cerr << "could not initialize image codecs\n";
        return 1;
    }
    shady::ChannelSet channels;
    const bool channels_ok = shady::load_channels(*options, channels, options->time);
    shady::quit_channel_images();
    if (!channels_ok) {
        return 1;
    }

    // render frame
    shady::GpuContext gpu;
    if (!shady::make_gpu_context(gpu)) {
        return 1;
    }
    shady::Renderer renderer;
    if (!renderer.init(gpu, options->width, options->height, *sources, channels) ||
        !renderer.render(make_frame(*options), channels)) {
        std::cerr << "could not render frame\n";
        return 1;
    }
    const auto rgba = renderer.read_rgba();
    if (!shady::write_ppm(options->output_path, options->width, options->height, rgba)) {
        std::cerr << "could not write " << options->output_path << '\n';
        return 1;
    }
    return 0;
}
