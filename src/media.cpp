#include "shady/channels.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace shady {
namespace {

std::uint32_t pack_rgba8(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                         std::uint8_t alpha = 255) {
    return (static_cast<std::uint32_t>(alpha) << 24U) |
           (static_cast<std::uint32_t>(blue) << 16U) |
           (static_cast<std::uint32_t>(green) << 8U) | static_cast<std::uint32_t>(red);
}

std::string shell_quote(const std::string& text) {
    std::string quoted = "'";
    for (const char value : text) {
        quoted += value == '\'' ? "'\\''" : std::string(1, value);
    }
    return quoted + "'";
}

std::optional<std::string> read_command(const std::string& command,
                                        std::optional<std::size_t> exact_size = std::nullopt) {
    FILE* pipe = popen(command.c_str(), "r");
    if (pipe == nullptr) {
        return std::nullopt;
    }
    std::string output;
    std::array<char, 4096> buffer{};
    while (true) {
        const std::size_t count = std::fread(buffer.data(), 1, buffer.size(), pipe);
        output.append(buffer.data(), count);
        if (count < buffer.size()) {
            break;
        }
    }
    const int status = pclose(pipe);
    if (status != 0 || (exact_size && output.size() != *exact_size)) {
        return std::nullopt;
    }
    return output;
}

double parse_rate(std::string_view text) {
    const std::size_t split = text.find('/');
    if (split == std::string_view::npos) {
        return std::max(1.0, std::atof(std::string{text}.c_str()));
    }
    const double numerator = std::atof(std::string{text.substr(0, split)}.c_str());
    const double denominator = std::atof(std::string{text.substr(split + 1)}.c_str());
    return numerator > 0.0 && denominator > 0.0 ? numerator / denominator : 30.0;
}

float audio_sample_at(const std::vector<float>& samples, std::size_t index) {
    return samples.empty() ? 0.0F : samples[index % samples.size()];
}

std::uint8_t audio_byte(float sample) {
    const float normalized = std::clamp(sample * 0.5F + 0.5F, 0.0F, 1.0F);
    return static_cast<std::uint8_t>(std::lround(normalized * 255.0F));
}

} // namespace

bool load_video_channel(const std::string& path, float time, ImageChannel& output) {
    const std::string quoted = shell_quote(path);
    const auto probe = read_command("ffprobe -v error -select_streams v:0 "
                                    "-show_entries stream=width,height,avg_frame_rate:format=duration "
                                    "-of default=noprint_wrappers=1 " +
                                    quoted);
    if (!probe) {
        std::cerr << "ffprobe failed for " << path << '\n';
        return false;
    }

    int width = 0;
    int height = 0;
    double rate = 30.0;
    double duration = 0.0;
    std::size_t offset = 0;
    while (offset < probe->size()) {
        const std::size_t end = probe->find('\n', offset);
        const std::string_view line{probe->data() + offset,
                                    (end == std::string::npos ? probe->size() : end) - offset};
        if (line.starts_with("width=")) {
            width = std::atoi(std::string{line.substr(6)}.c_str());
        } else if (line.starts_with("height=")) {
            height = std::atoi(std::string{line.substr(7)}.c_str());
        } else if (line.starts_with("avg_frame_rate=")) {
            rate = parse_rate(line.substr(15));
        } else if (line.starts_with("duration=")) {
            duration = std::atof(std::string{line.substr(9)}.c_str());
        }
        if (end == std::string::npos) {
            break;
        }
        offset = end + 1;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }

    const double playback = duration > 0.0 ? std::fmod(std::max(0.0F, time), duration) : time;
    const double snapped = std::floor(playback * rate) / rate;
    const std::size_t bytes = static_cast<std::size_t>(width * height) * 4U;
    const auto raw = read_command("ffmpeg -v error -ss " + std::to_string(snapped) + " -i " +
                                      quoted + " -frames:v 1 -f rawvideo -pix_fmt rgba - 2>/dev/null",
                                  bytes);
    if (!raw) {
        std::cerr << "ffmpeg video decode failed for " << path << '\n';
        return false;
    }

    output.width = width;
    output.height = height;
    output.time = static_cast<float>(snapped);
    output.sample_rate = 0.0F;
    output.pixels.resize(static_cast<std::size_t>(width * height));
    for (std::size_t pixel = 0; pixel < output.pixels.size(); ++pixel) {
        output.pixels[pixel] = pack_rgba8(
            static_cast<std::uint8_t>((*raw)[pixel * 4U]),
            static_cast<std::uint8_t>((*raw)[pixel * 4U + 1U]),
            static_cast<std::uint8_t>((*raw)[pixel * 4U + 2U]),
            static_cast<std::uint8_t>((*raw)[pixel * 4U + 3U]));
    }
    return true;
}

void build_audio_texture(const std::vector<float>& samples, int sample_rate, double time,
                         std::vector<std::uint32_t>& pixels) {
    pixels.resize(static_cast<std::size_t>(audio_texture_width * audio_texture_height));
    const auto center = static_cast<long long>(std::max(0.0, time) * sample_rate);
    for (int x = 0; x < audio_texture_width; ++x) {
        const long long source = std::max(0LL, center - audio_texture_width / 2 + x);
        const std::uint8_t value = audio_byte(audio_sample_at(samples, source));
        pixels[static_cast<std::size_t>(x)] = pack_rgba8(value, value, value);
    }

    constexpr int bins = 128;
    constexpr int window = 512;
    constexpr float tau = 6.28318530717958647692F;
    std::array<float, bins> magnitudes{};
    for (int bin = 0; bin < bins; ++bin) {
        float real = 0.0F;
        float imaginary = 0.0F;
        for (int index = 0; index < window; ++index) {
            const long long source = std::max(0LL, center - window + index);
            const float sample = audio_sample_at(samples, source);
            const float phase = tau * static_cast<float>(bin + 1) *
                                static_cast<float>(index) / static_cast<float>(window);
            real += sample * std::cos(phase);
            imaginary -= sample * std::sin(phase);
        }
        magnitudes[bin] = std::clamp(std::sqrt(real * real + imaginary * imaginary) / 32.0F,
                                     0.0F, 1.0F);
    }
    for (int x = 0; x < audio_texture_width; ++x) {
        const int bin = x * bins / audio_texture_width;
        const auto value = static_cast<std::uint8_t>(std::lround(magnitudes[bin] * 255.0F));
        pixels[static_cast<std::size_t>(audio_texture_width + x)] =
            pack_rgba8(value, value, value);
    }
}

bool load_audio_channel(const std::string& path, float time, ImageChannel& output) {
    const auto raw = read_command("ffmpeg -v error -i " + shell_quote(path) +
                                  " -ac 1 -ar 44100 -f f32le -");
    if (!raw || raw->size() < sizeof(float)) {
        std::cerr << "ffmpeg audio decode failed for " << path << '\n';
        return false;
    }
    std::vector<float> samples(raw->size() / sizeof(float));
    std::memcpy(samples.data(), raw->data(), samples.size() * sizeof(float));
    const double duration = static_cast<double>(samples.size()) / audio_sample_rate;
    const double playback = duration > 0.0 ? std::fmod(std::max(0.0F, time), duration) : 0.0;
    output.width = audio_texture_width;
    output.height = audio_texture_height;
    output.time = static_cast<float>(playback);
    output.sample_rate = audio_sample_rate;
    build_audio_texture(samples, audio_sample_rate, playback, output.pixels);
    return true;
}

} // namespace shady
