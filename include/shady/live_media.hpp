#pragma once

#include "shady/model.hpp"

#include <array>
#include <cstdio>
#include <vector>

namespace shady {

class LiveMedia {
  public:
    ~LiveMedia();
    bool start(const AppOptions& options, ChannelSet& channels);
    bool update(float time, ChannelSet& channels);

  private:
    struct Stream {
        FILE* pipe = nullptr;
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> pending;
        std::vector<float> samples;
        std::size_t used = 0;
        bool microphone = false;
    };
    std::array<Stream, channel_count> streams_;

    void close();
};

} // namespace shady
