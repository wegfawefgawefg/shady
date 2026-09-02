#include "shady/live_media.hpp"

#include "shady/channels.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <unistd.h>

namespace shady {
namespace {

std::string shell_quote(const std::string& text) {
    std::string quoted = "'";
    for (const char value : text) quoted += value == '\'' ? "'\\''" : std::string(1, value);
    return quoted + "'";
}

bool nonblocking(FILE* pipe) {
    const int fd = fileno(pipe);
    const int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool read_exact(FILE* pipe, std::vector<std::uint8_t>& bytes) {
    std::size_t used = 0;
    while (used < bytes.size()) {
        const ssize_t count = read(fileno(pipe), bytes.data() + used, bytes.size() - used);
        if (count > 0) {
            used += static_cast<std::size_t>(count);
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

void pack_webcam(const std::vector<std::uint8_t>& source, int width, int height,
                 ImageChannel& channel) {
    channel.width = width;
    channel.height = height;
    channel.sample_rate = 0.0F;
    channel.pixels.resize(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t from = static_cast<std::size_t>(y * width + width - x - 1) * 4U;
            const std::uint32_t red = source[from];
            const std::uint32_t green = source[from + 1U];
            const std::uint32_t blue = source[from + 2U];
            channel.pixels[static_cast<std::size_t>(y * width + x)] =
                0xFF000000U | (blue << 16U) | (green << 8U) | red;
        }
    }
}

} // namespace

LiveMedia::~LiveMedia() {
    close();
}

bool LiveMedia::start(const AppOptions& options, ChannelSet& channels) {
    for (std::size_t index = 0; index < streams_.size(); ++index) {
        Stream& stream = streams_[index];
        if (!options.webcam_paths[index].empty()) {
            stream.width = 320;
            stream.height = 240;
            stream.pending.resize(static_cast<std::size_t>(stream.width * stream.height) * 4U);
            const std::string command =
                "ffmpeg -v error -fflags nobuffer -flags low_delay -f v4l2 -framerate 30 "
                "-video_size 320x240 -i " + shell_quote(options.webcam_paths[index]) +
                " -f rawvideo -pix_fmt rgba - 2>/dev/null";
            stream.pipe = popen(command.c_str(), "r");
            if (stream.pipe == nullptr || !read_exact(stream.pipe, stream.pending)) return false;
            pack_webcam(stream.pending, stream.width, stream.height, channels.image[index]);
            stream.used = 0;
            if (!nonblocking(stream.pipe)) return false;
        } else if (!options.microphone_paths[index].empty()) {
            stream.microphone = true;
            stream.samples.assign(audio_texture_width, 0.0F);
            stream.pending.resize(4096);
            const std::string command = "ffmpeg -v error -f pulse -i " +
                shell_quote(options.microphone_paths[index]) +
                " -ac 1 -ar 44100 -f f32le - 2>/dev/null";
            stream.pipe = popen(command.c_str(), "r");
            if (stream.pipe == nullptr || !nonblocking(stream.pipe)) return false;
            ImageChannel& channel = channels.image[index];
            channel.width = audio_texture_width;
            channel.height = audio_texture_height;
            channel.sample_rate = audio_sample_rate;
            build_audio_texture(stream.samples, audio_sample_rate, 0.0, channel.pixels);
        }
    }
    return true;
}

bool LiveMedia::update(float time, ChannelSet& channels) {
    for (std::size_t index = 0; index < streams_.size(); ++index) {
        Stream& stream = streams_[index];
        if (stream.pipe == nullptr) continue;
        if (stream.microphone) {
            while (true) {
                const ssize_t count = read(fileno(stream.pipe), stream.pending.data(),
                                           stream.pending.size());
                if (count > 0) {
                    const std::size_t sample_count = static_cast<std::size_t>(count) / sizeof(float);
                    const std::size_t old = stream.samples.size();
                    stream.samples.resize(old + sample_count);
                    std::memcpy(stream.samples.data() + old, stream.pending.data(),
                                sample_count * sizeof(float));
                    if (stream.samples.size() > audio_texture_width) {
                        stream.samples.erase(stream.samples.begin(), stream.samples.end() -
                            static_cast<std::ptrdiff_t>(audio_texture_width));
                    }
                    continue;
                }
                if (count == 0) return false;
                if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return false;
                break;
            }
            ImageChannel& channel = channels.image[index];
            channel.time = time;
            build_audio_texture(stream.samples, audio_sample_rate, time, channel.pixels);
            continue;
        }

        const std::size_t frame_bytes = stream.pending.size();
        while (true) {
            const ssize_t count = read(fileno(stream.pipe), stream.pending.data() + stream.used,
                                       frame_bytes - stream.used);
            if (count > 0) {
                stream.used += static_cast<std::size_t>(count);
                if (stream.used == frame_bytes) {
                    pack_webcam(stream.pending, stream.width, stream.height, channels.image[index]);
                    stream.used = 0;
                }
                continue;
            }
            if (count == 0) return false;
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) return false;
            break;
        }
        channels.image[index].time = time;
    }
    return true;
}

void LiveMedia::close() {
    for (Stream& stream : streams_) {
        if (stream.pipe != nullptr) {
            pclose(stream.pipe);
            stream.pipe = nullptr;
        }
    }
}

} // namespace shady
