#pragma once

#include "shady/model.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <webgpu/webgpu.hpp>

namespace shady {

struct GpuContext {
    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
    wgpu::Queue queue;
};

bool make_gpu_context(GpuContext& context);

class Renderer {
  public:
    bool init(GpuContext& context, int width, int height, const ShaderSources& sources,
              const ChannelSet& channels);
    bool reload(const ShaderSources& sources);
    bool render(const FrameState& frame, const ChannelSet& channels);
    bool upload_channel(std::size_t index, const ImageChannel& channel);
    [[nodiscard]] wgpu::TextureView output_view() const;
    [[nodiscard]] wgpu::Texture output_texture() const;
    [[nodiscard]] std::vector<std::uint8_t> read_rgba();

  private:
    struct Pass {
        bool active = false;
        wgpu::ComputePipeline pipeline;
    };

    GpuContext* context_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int buffer_read_index_ = 0;
    wgpu::BindGroupLayout bind_group_layout_;
    wgpu::PipelineLayout pipeline_layout_;
    wgpu::Buffer uniform_buffer_;
    Pass image_pass_;
    std::array<Pass, channel_count> buffer_passes_;
    std::array<wgpu::Texture, channel_count> channel_textures_;
    std::array<wgpu::TextureView, channel_count> channel_views_;
    std::array<std::array<wgpu::Texture, 2>, channel_count> buffer_textures_;
    std::array<std::array<wgpu::TextureView, 2>, channel_count> buffer_views_;
    wgpu::Texture output_texture_;
    wgpu::TextureView output_view_;

    bool make_layout();
    bool make_targets(const ChannelSet& channels);
    Pass make_pass(const std::string& source, bool active);
    bool dispatch(const Pass& pass, const FrameState& frame, const ChannelSet& channels,
                  const std::array<wgpu::TextureView, channel_count>& views,
                  wgpu::TextureView output);
};

bool write_ppm(const std::string& path, int width, int height,
               const std::vector<std::uint8_t>& rgba);

} // namespace shady
