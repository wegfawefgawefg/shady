#pragma once

#include "shady/gpu.hpp"

#include <SDL.h>
#include <optional>

namespace shady {

class Presentation {
  public:
    bool init(GpuContext& context, SDL_Window* window, int width, int height, int scale);
    bool present(wgpu::TextureView source);
    void shutdown();

  private:
    GpuContext* context_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    int scale_ = 1;
    wgpu::Surface surface_;
    wgpu::RenderPipeline pipeline_;
    wgpu::BindGroupLayout layout_;
    wgpu::Buffer uniform_;
};

} // namespace shady
