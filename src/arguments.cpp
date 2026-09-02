#include "shady/arguments.hpp"

#include <array>
#include <cstdlib>
#include <string>

namespace shady {
namespace {

struct SizePreset {
    std::string_view name;
    int width;
    int height;
};

constexpr std::array presets{
    SizePreset{"gb", 160, 144},       SizePreset{"gba", 240, 160},
    SizePreset{"nes", 256, 240},     SizePreset{"snes", 256, 224},
    SizePreset{"genesis", 320, 224}, SizePreset{"n64", 320, 240},
    SizePreset{"ps1", 320, 240},     SizePreset{"psp", 480, 272},
};

bool set_channel_path(std::string_view flag, std::string_view prefix,
                      std::array<std::string, channel_count>& paths, const std::string& value) {
    if (!flag.starts_with(prefix) || flag.size() != prefix.size() + 1) {
        return false;
    }
    const int index = static_cast<int>(flag.back() - '0');
    if (index < 0 || index >= channel_count) {
        return false;
    }
    paths[static_cast<std::size_t>(index)] = value;
    return true;
}

} // namespace

std::optional<std::pair<int, int>> parse_size(std::string_view text) {
    for (const SizePreset& preset : presets) {
        if (text == preset.name) {
            return std::pair{preset.width, preset.height};
        }
    }
    const std::size_t split = text.find('x');
    if (split == std::string_view::npos) {
        return std::nullopt;
    }
    const int width = std::atoi(std::string{text.substr(0, split)}.c_str());
    const int height = std::atoi(std::string{text.substr(split + 1)}.c_str());
    if (width <= 0 || height <= 0) {
        return std::nullopt;
    }
    return std::pair{width, height};
}

std::optional<AppOptions> parse_arguments(int argc, char** argv, bool headless) {
    AppOptions options;
    options.headless = headless;
    for (int index = 1; index < argc; ++index) {
        const std::string_view flag{argv[index]};
        auto next = [&]() -> std::optional<std::string> {
            if (index + 1 >= argc) {
                return std::nullopt;
            }
            return std::string{argv[++index]};
        };

        if (flag == "--help" || flag == "-h") {
            options.show_help = true;
            continue;
        }
        if (flag == "--size") {
            const auto value = next();
            const auto size = value ? parse_size(*value) : std::nullopt;
            if (!size) {
                return std::nullopt;
            }
            options.width = size->first;
            options.height = size->second;
            continue;
        }
        if (flag == "--scale" || flag == "--dimscale") {
            const auto value = next();
            options.scale = value ? std::atoi(value->c_str()) : 0;
            if (options.scale <= 0) {
                return std::nullopt;
            }
            continue;
        }
        if (flag == "--frames") {
            const auto value = next();
            options.frames = value ? std::atoi(value->c_str()) : 0;
            if (options.frames <= 0) {
                return std::nullopt;
            }
            continue;
        }
        if (flag == "--frame") {
            const auto value = next();
            options.frame = value ? std::atoi(value->c_str()) : -1;
            if (options.frame < 0) {
                return std::nullopt;
            }
            continue;
        }
        if (flag == "--time") {
            const auto value = next();
            if (!value) {
                return std::nullopt;
            }
            options.time = std::strtof(value->c_str(), nullptr);
            continue;
        }
        if (flag == "--time-delta") {
            const auto value = next();
            if (!value) {
                return std::nullopt;
            }
            options.time_delta = std::strtof(value->c_str(), nullptr);
            continue;
        }
        if (flag == "--output" || flag == "-o") {
            const auto value = next();
            if (!value) {
                return std::nullopt;
            }
            options.output_path = *value;
            continue;
        }
        if (flag.starts_with("--buffer") && flag.size() == 9) {
            const auto value = next();
            if (!value || !set_channel_path(flag, "--buffer", options.project.buffer_paths, *value)) {
                return std::nullopt;
            }
            continue;
        }
        auto channel_option = [&](std::string_view prefix,
                                  std::array<std::string, channel_count>& paths) {
            if (!flag.starts_with(prefix)) {
                return false;
            }
            const auto value = next();
            return value && set_channel_path(flag, prefix, paths, *value);
        };
        if (flag.starts_with("--channel")) {
            if (!channel_option("--channel", options.image_paths)) return std::nullopt;
            continue;
        }
        if (flag.starts_with("--noise")) {
            if (!channel_option("--noise", options.noise_seeds)) return std::nullopt;
            continue;
        }
        if (flag.starts_with("--video")) {
            if (!channel_option("--video", options.video_paths)) return std::nullopt;
            continue;
        }
        if (flag.starts_with("--webcam")) {
            if (!channel_option("--webcam", options.webcam_paths)) return std::nullopt;
            continue;
        }
        if (flag.starts_with("--audio")) {
            if (!channel_option("--audio", options.audio_paths)) return std::nullopt;
            continue;
        }
        if (flag.starts_with("--mic")) {
            if (!channel_option("--mic", options.microphone_paths)) return std::nullopt;
            continue;
        }
        if (!flag.empty() && flag.front() != '-' && options.project.image_path.empty()) {
            options.project.image_path = std::string{flag};
            continue;
        }
        return std::nullopt;
    }

    if (!options.show_help && options.project.image_path.empty()) {
        return std::nullopt;
    }
    if (headless && !options.show_help && options.output_path.empty()) {
        return std::nullopt;
    }
    return options;
}

void print_usage(std::ostream& output, bool headless) {
    output << (headless ? "usage: shady-frame shader.wgsl --output frame.ppm"
                        : "usage: shady shader.wgsl")
           << " [--size gba|640x420] [--scale N] [--frames N]\n"
           << "       [--time seconds] [--time-delta seconds] [--buffer0 pass.wgsl]\n"
           << "       [--channel0 image] [--noise0 seed] [--video0 file] [--audio0 file]\n"
           << "       [--webcam0 device] [--mic0 device]\n";
}

} // namespace shady
