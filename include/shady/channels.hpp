#pragma once

#include "shady/model.hpp"

#include <string>

namespace shady {

bool init_channel_images();
void quit_channel_images();
bool load_image_channel(const std::string& path, ImageChannel& output);
void load_noise_channel(const std::string& seed, ImageChannel& output);
bool load_video_channel(const std::string& path, float time, ImageChannel& output);
bool load_audio_channel(const std::string& path, float time, ImageChannel& output);
bool load_channels(const AppOptions& options, ChannelSet& channels, float time);
bool write_png(const std::string& path, int width, int height,
               const std::vector<std::uint8_t>& rgba);
void build_audio_texture(const std::vector<float>& samples, int sample_rate, double time,
                         std::vector<std::uint32_t>& pixels);

} // namespace shady
