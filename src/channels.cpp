#include "shady/channels.hpp"

#include <SDL.h>
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

namespace shady {
namespace {

std::uint32_t pack_rgba8(std::uint8_t red, std::uint8_t green, std::uint8_t blue,
                         std::uint8_t alpha = 255) {
    return (static_cast<std::uint32_t>(alpha) << 24U) |
           (static_cast<std::uint32_t>(blue) << 16U) |
           (static_cast<std::uint32_t>(green) << 8U) | static_cast<std::uint32_t>(red);
}

std::uint32_t hash_u32(std::uint32_t value) {
    value ^= value >> 16U;
    value *= 0x7feb352dU;
    value ^= value >> 15U;
    value *= 0x846ca68bU;
    value ^= value >> 16U;
    return value;
}

std::uint32_t seed_from_text(const std::string& text) {
    std::uint32_t seed = 2166136261U;
    for (const unsigned char value : text) {
        seed ^= static_cast<std::uint32_t>(value);
        seed *= 16777619U;
    }
    return seed;
}

} // namespace

bool init_channel_images() {
    const int requested = IMG_INIT_JPG | IMG_INIT_PNG;
    return (IMG_Init(requested) & requested) != 0;
}

void quit_channel_images() {
    IMG_Quit();
}

bool load_image_channel(const std::string& path, ImageChannel& output) {
    SDL_Surface* loaded = IMG_Load(path.c_str());
    if (loaded == nullptr) {
        std::cerr << "IMG_Load failed for " << path << ": " << IMG_GetError() << '\n';
        return false;
    }

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(loaded);
    if (converted == nullptr) {
        std::cerr << "image conversion failed for " << path << ": " << SDL_GetError() << '\n';
        return false;
    }

    output.width = converted->w;
    output.height = converted->h;
    output.time = 0.0F;
    output.sample_rate = 0.0F;
    output.pixels.resize(static_cast<std::size_t>(output.width * output.height));
    const auto* bytes = static_cast<const std::uint8_t*>(converted->pixels);
    for (int y = 0; y < output.height; ++y) {
        const auto* row = reinterpret_cast<const std::uint32_t*>(
            bytes + static_cast<std::size_t>(y) * static_cast<std::size_t>(converted->pitch));
        std::copy_n(row, output.width,
                    output.pixels.begin() + static_cast<std::ptrdiff_t>(y * output.width));
    }
    SDL_FreeSurface(converted);
    return true;
}

void load_noise_channel(const std::string& seed_text, ImageChannel& output) {
    constexpr int size = 256;
    const std::uint32_t seed = seed_from_text(seed_text);
    output.width = size;
    output.height = size;
    output.time = 0.0F;
    output.sample_rate = 0.0F;
    output.pixels.resize(static_cast<std::size_t>(size * size));
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const std::uint32_t base = seed ^ (static_cast<std::uint32_t>(x) * 374761393U) ^
                                       (static_cast<std::uint32_t>(y) * 668265263U);
            const auto red = static_cast<std::uint8_t>(hash_u32(base) & 0xFFU);
            const auto green = static_cast<std::uint8_t>(hash_u32(base + 1U) & 0xFFU);
            const auto blue = static_cast<std::uint8_t>(hash_u32(base + 2U) & 0xFFU);
            output.pixels[static_cast<std::size_t>(y * size + x)] =
                pack_rgba8(red, green, blue);
        }
    }
}

bool load_channels(const AppOptions& options, ChannelSet& channels, float time) {
    for (std::size_t index = 0; index < channels.image.size(); ++index) {
        if (!options.image_paths[index].empty() &&
            !load_image_channel(options.image_paths[index], channels.image[index])) {
            return false;
        }
        if (!options.noise_seeds[index].empty()) {
            load_noise_channel(options.noise_seeds[index], channels.image[index]);
        }
        if (!options.video_paths[index].empty() &&
            !load_video_channel(options.video_paths[index], time, channels.image[index])) {
            return false;
        }
        if (!options.audio_paths[index].empty() &&
            !load_audio_channel(options.audio_paths[index], time, channels.image[index])) {
            return false;
        }
    }
    return true;
}

bool write_png(const std::string& path, int width, int height,
               const std::vector<std::uint8_t>& rgba) {
    if (rgba.size() != static_cast<std::size_t>(width * height) * 4U) return false;
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<std::uint8_t*>(rgba.data()), width, height, 32, width * 4,
        SDL_PIXELFORMAT_ABGR8888);
    if (surface == nullptr) return false;
    const bool written = IMG_SavePNG(surface, path.c_str()) == 0;
    SDL_FreeSurface(surface);
    return written;
}

} // namespace shady
