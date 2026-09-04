#include "shady/presentation.hpp"

#include <SDL_syswm.h>
#include <array>
#include <cstdint>
#include <iostream>

namespace shady {
namespace {

constexpr const char* present_source = R"wgsl(
struct PresentUniforms { source_size_scale: vec4<f32> };
@group(0) @binding(0) var source_texture: texture_2d<f32>;
@group(0) @binding(1) var<uniform> present: PresentUniforms;
struct VertexOut { @builtin(position) position: vec4<f32> };

@vertex fn vs(@builtin(vertex_index) index: u32) -> VertexOut {
    var points = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0), vec2<f32>(3.0, -1.0), vec2<f32>(-1.0, 3.0));
    var out: VertexOut;
    out.position = vec4<f32>(points[index], 0.0, 1.0);
    return out;
}

@fragment fn fs(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
    let size = vec2<i32>(present.source_size_scale.xy);
    let scale = present.source_size_scale.z;
    let coord = clamp(vec2<i32>(floor(position.xy / scale)), vec2<i32>(0), size - 1);
    return textureLoad(source_texture, coord, 0);
}
)wgsl";

wgpu::ShaderModule shader_module(wgpu::Device device, const char* source) {
    wgpu::ShaderModuleWGSLDescriptor wgsl{};
    wgsl.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    wgsl.code = source;
    wgpu::ShaderModuleDescriptor descriptor{};
    descriptor.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl.chain);
    return device.createShaderModule(descriptor);
}

std::optional<wgpu::Surface> make_surface(wgpu::Instance instance, SDL_Window* window) {
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (SDL_GetWindowWMInfo(window, &info) != SDL_TRUE || info.subsystem != SDL_SYSWM_X11) {
        std::cerr << "native windows currently require SDL on X11\n";
        return std::nullopt;
    }
    wgpu::SurfaceDescriptorFromXlibWindow xlib{};
    xlib.chain.sType = wgpu::SType::SurfaceDescriptorFromXlibWindow;
    xlib.display = info.info.x11.display;
    xlib.window = static_cast<std::uint64_t>(info.info.x11.window);
    wgpu::SurfaceDescriptor descriptor{};
    descriptor.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&xlib.chain);
    wgpu::Surface surface = instance.createSurface(descriptor);
    return surface == WGPUSurface(nullptr) ? std::nullopt : std::optional{surface};
}

wgpu::TextureFormat surface_format(wgpu::Surface surface, wgpu::Adapter adapter) {
    wgpu::SurfaceCapabilities capabilities{};
    surface.getCapabilities(adapter, &capabilities);
    wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
    for (std::size_t index = 0; index < capabilities.formatCount; ++index) {
        const auto candidate = capabilities.formats[index];
        if (candidate == wgpu::TextureFormat::BGRA8Unorm ||
            candidate == wgpu::TextureFormat::RGBA8Unorm) {
            format = candidate;
            break;
        }
    }
    if (format == wgpu::TextureFormat::Undefined && capabilities.formatCount > 0) {
        format = capabilities.formats[0];
    }
    capabilities.freeMembers();
    if (format != wgpu::TextureFormat::Undefined) return format;
    const auto preferred = surface.getPreferredFormat(adapter);
    if (preferred != wgpu::TextureFormat::Undefined) return preferred;
    return wgpu::TextureFormat::BGRA8Unorm;
}

} // namespace

bool Presentation::init(GpuContext& context, SDL_Window* window, int width, int height, int scale) {
    const auto made_surface = make_surface(context.instance, window);
    if (!made_surface) return false;
    context_ = &context;
    width_ = width;
    height_ = height;
    scale_ = scale;
    surface_ = *made_surface;
    const auto format = surface_format(surface_, context.adapter);
    wgpu::SurfaceConfiguration config{};
    config.device = context.device;
    config.format = format;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.width = static_cast<std::uint32_t>(width * scale);
    config.height = static_cast<std::uint32_t>(height * scale);
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    surface_.configure(config);

    std::array<wgpu::BindGroupLayoutEntry, 2> entries{};
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Fragment;
    entries[0].texture.sampleType = wgpu::TextureSampleType::Float;
    entries[0].texture.viewDimension = wgpu::TextureViewDimension::_2D;
    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Fragment;
    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;
    wgpu::BindGroupLayoutDescriptor layout_descriptor{};
    layout_descriptor.entryCount = entries.size();
    layout_descriptor.entries = entries.data();
    layout_ = context.device.createBindGroupLayout(layout_descriptor);
    WGPUBindGroupLayout raw_layout = layout_;
    wgpu::PipelineLayoutDescriptor pipeline_layout_descriptor{};
    pipeline_layout_descriptor.bindGroupLayoutCount = 1;
    pipeline_layout_descriptor.bindGroupLayouts = &raw_layout;
    const auto pipeline_layout = context.device.createPipelineLayout(pipeline_layout_descriptor);
    const auto shader = shader_module(context.device, present_source);
    wgpu::ColorTargetState target{};
    target.format = format;
    target.writeMask = wgpu::ColorWriteMask::All;
    wgpu::FragmentState fragment{};
    fragment.module = shader;
    fragment.entryPoint = "fs";
    fragment.targetCount = 1;
    fragment.targets = &target;
    wgpu::RenderPipelineDescriptor pipeline_descriptor{};
    pipeline_descriptor.layout = pipeline_layout;
    pipeline_descriptor.vertex.module = shader;
    pipeline_descriptor.vertex.entryPoint = "vs";
    pipeline_descriptor.fragment = &fragment;
    pipeline_descriptor.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    pipeline_descriptor.multisample.count = 1;
    pipeline_descriptor.multisample.mask = 0xffffffffU;
    pipeline_ = context.device.createRenderPipeline(pipeline_descriptor);
    wgpu::BufferDescriptor buffer_descriptor{};
    buffer_descriptor.size = 16;
    buffer_descriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniform_ = context.device.createBuffer(buffer_descriptor);
    return pipeline_ != WGPURenderPipeline(nullptr);
}

bool Presentation::present(wgpu::TextureView source) {
    const std::array<float, 4> values{static_cast<float>(width_), static_cast<float>(height_),
                                      static_cast<float>(scale_), 0.0F};
    context_->queue.writeBuffer(uniform_, 0, values.data(), sizeof(values));
    wgpu::SurfaceTexture texture{};
    surface_.getCurrentTexture(&texture);
    if (texture.status != WGPUSurfaceGetCurrentTextureStatus_Success ||
        texture.texture == WGPUTexture(nullptr)) return false;
    wgpu::Texture handle{texture.texture};
    const auto surface_view = handle.createView();
    std::array<wgpu::BindGroupEntry, 2> entries{};
    entries[0].binding = 0;
    entries[0].textureView = source;
    entries[1].binding = 1;
    entries[1].buffer = uniform_;
    entries[1].size = 16;
    wgpu::BindGroupDescriptor bind_descriptor{};
    bind_descriptor.layout = layout_;
    bind_descriptor.entryCount = entries.size();
    bind_descriptor.entries = entries.data();
    const auto bind_group = context_->device.createBindGroup(bind_descriptor);
    wgpu::RenderPassColorAttachment attachment{};
    attachment.view = surface_view;
    attachment.loadOp = wgpu::LoadOp::Clear;
    attachment.storeOp = wgpu::StoreOp::Store;
    attachment.clearValue = wgpu::Color{0.835, 0.855, 0.84, 1.0};
    wgpu::RenderPassDescriptor pass_descriptor{};
    pass_descriptor.colorAttachmentCount = 1;
    pass_descriptor.colorAttachments = &attachment;
    auto encoder = context_->device.createCommandEncoder();
    auto pass = encoder.beginRenderPass(pass_descriptor);
    pass.setPipeline(pipeline_);
    pass.setBindGroup(0, bind_group, 0, nullptr);
    pass.draw(3, 1, 0, 0);
    pass.end();
    context_->queue.submit(encoder.finish());
    surface_.present();
    return true;
}

void Presentation::shutdown() {
    if (surface_ != WGPUSurface(nullptr)) surface_.unconfigure();
}

} // namespace shady
