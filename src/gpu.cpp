#define WEBGPU_CPP_IMPLEMENTATION
#include "shady/gpu.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>

namespace shady {
namespace {

constexpr std::uint64_t channel_offset = 16;
constexpr std::uint64_t uniform_float_count = 16 + 16 + 512 + 8 + 4 + 32 + 16;
constexpr std::uint64_t uniform_payload_size = uniform_float_count * sizeof(float);
constexpr std::uint64_t uniform_buffer_size = (uniform_payload_size + 15U) & ~std::uint64_t{15};

std::uint32_t align_to(std::uint32_t value, std::uint32_t alignment) {
    return ((value + alignment - 1U) / alignment) * alignment;
}

wgpu::ShaderModule make_shader_module(wgpu::Device device, const std::string& source) {
    wgpu::ShaderModuleWGSLDescriptor wgsl{};
    wgsl.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    wgsl.code = source.c_str();
    wgpu::ShaderModuleDescriptor descriptor{};
    descriptor.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgsl.chain);
    return device.createShaderModule(descriptor);
}

wgpu::Texture make_texture(GpuContext& context, int width, int height,
                           WGPUTextureUsageFlags usage) {
    wgpu::TextureDescriptor descriptor{wgpu::Default};
    descriptor.size = wgpu::Extent3D(static_cast<std::uint32_t>(width),
                                     static_cast<std::uint32_t>(height), 1);
    descriptor.format = wgpu::TextureFormat::RGBA8Unorm;
    descriptor.usage = usage;
    return context.device.createTexture(descriptor);
}

void clear_texture(GpuContext& context, wgpu::Texture texture, int width, int height) {
    const std::uint32_t row_size = static_cast<std::uint32_t>(width) * 4U;
    const std::uint32_t padded_row_size = align_to(row_size, 256);
    const std::vector<std::uint8_t> zeroes(static_cast<std::size_t>(padded_row_size) * height);
    wgpu::ImageCopyTexture destination{};
    destination.texture = texture;
    wgpu::TextureDataLayout layout{};
    layout.bytesPerRow = padded_row_size;
    layout.rowsPerImage = static_cast<std::uint32_t>(height);
    context.queue.writeTexture(destination, zeroes.data(), zeroes.size(), layout,
                               wgpu::Extent3D(static_cast<std::uint32_t>(width),
                                              static_cast<std::uint32_t>(height), 1));
}

void write_texture(GpuContext& context, wgpu::Texture texture, const ImageChannel& channel) {
    if (!channel.loaded()) {
        return;
    }
    constexpr std::uint32_t alignment = 256;
    const std::uint32_t row_size = static_cast<std::uint32_t>(channel.width) * 4U;
    const std::uint32_t padded_row_size = align_to(row_size, alignment);
    std::vector<std::uint8_t> padded(static_cast<std::size_t>(padded_row_size) * channel.height);
    for (int y = 0; y < channel.height; ++y) {
        std::memcpy(padded.data() + static_cast<std::size_t>(y) * padded_row_size,
                    channel.pixels.data() + static_cast<std::size_t>(y * channel.width), row_size);
    }
    wgpu::ImageCopyTexture destination{};
    destination.texture = texture;
    wgpu::TextureDataLayout layout{};
    layout.bytesPerRow = padded_row_size;
    layout.rowsPerImage = static_cast<std::uint32_t>(channel.height);
    context.queue.writeTexture(destination, padded.data(), padded.size(), layout,
                               wgpu::Extent3D(static_cast<std::uint32_t>(channel.width),
                                              static_cast<std::uint32_t>(channel.height), 1));
}

wgpu::Texture make_channel_texture(GpuContext& context, const ImageChannel& channel) {
    const int width = channel.loaded() ? channel.width : 1;
    const int height = channel.loaded() ? channel.height : 1;
    wgpu::Texture texture = make_texture(context, width, height,
                                         wgpu::TextureUsage::TextureBinding |
                                             wgpu::TextureUsage::CopyDst);
    if (channel.loaded()) {
        write_texture(context, texture, channel);
    } else {
        const std::array<std::uint8_t, 4> zero{};
        wgpu::ImageCopyTexture destination{};
        destination.texture = texture;
        wgpu::TextureDataLayout layout{};
        layout.bytesPerRow = 256;
        layout.rowsPerImage = 1;
        context.queue.writeTexture(destination, zero.data(), zero.size(), layout,
                                   wgpu::Extent3D(1, 1, 1));
    }
    return texture;
}

std::vector<float> make_uniforms(const FrameState& frame, const ChannelSet& channels) {
    std::vector<float> values(uniform_float_count, 0.0F);
    values[0] = frame.time;
    values[1] = frame.time_delta;
    values[2] = static_cast<float>(frame.frame);
    values[3] = static_cast<float>(frame.width);
    values[4] = static_cast<float>(frame.height);
    values[5] = frame.input.mouse_x;
    values[6] = frame.input.mouse_y;
    values[7] = frame.input.mouse_down;
    values[8] = frame.input.mouse_click_x;
    values[9] = frame.input.mouse_click_y;
    values[10] = frame.wall_clock_seconds;
    values[11] = static_cast<float>(frame.year);
    values[12] = static_cast<float>(frame.month);
    values[13] = static_cast<float>(frame.day);
    for (std::size_t index = 0; index < channels.image.size(); ++index) {
        const ImageChannel& channel = channels.image[index];
        const std::size_t offset = channel_offset + index * 4U;
        values[offset] = static_cast<float>(std::max(1, channel.width));
        values[offset + 1U] = static_cast<float>(std::max(1, channel.height));
        values[offset + 2U] = channel.time;
        values[offset + 3U] = channel.sample_rate;
    }
    std::copy(frame.input.keys.begin(), frame.input.keys.end(), values.begin() + 32);
    std::copy(frame.input.mouse_buttons.begin(), frame.input.mouse_buttons.end(),
              values.begin() + 544);
    values[552] = frame.input.mouse_wheel_x;
    values[553] = frame.input.mouse_wheel_y;
    std::copy(frame.input.gamepad_buttons.begin(), frame.input.gamepad_buttons.end(),
              values.begin() + 556);
    std::copy(frame.input.gamepad_axes.begin(), frame.input.gamepad_axes.end(),
              values.begin() + 588);
    return values;
}

std::vector<std::uint8_t> map_readback(wgpu::Device device, wgpu::Buffer buffer,
                                       std::uint64_t size) {
    bool complete = false;
    bool failed = false;
    auto callback = buffer.mapAsync(wgpu::MapMode::Read, 0, size,
                                    [&](wgpu::BufferMapAsyncStatus status) {
                                        complete = true;
                                        failed = status != wgpu::BufferMapAsyncStatus::Success;
                                    });
    while (!complete) {
        device.poll(true);
    }
    if (failed) {
        return {};
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    std::memcpy(bytes.data(), buffer.getConstMappedRange(0, size), bytes.size());
    buffer.unmap();
    (void)callback;
    return bytes;
}

} // namespace

bool make_gpu_context(GpuContext& context) {
    context.instance = wgpu::createInstance({});
    if (context.instance == WGPUInstance(nullptr)) {
        std::cerr << "could not create WebGPU instance\n";
        return false;
    }
    context.adapter = context.instance.requestAdapter({});
    if (context.adapter == WGPUAdapter(nullptr)) {
        std::cerr << "could not request WebGPU adapter\n";
        return false;
    }
    context.device = context.adapter.requestDevice({});
    if (context.device == WGPUDevice(nullptr)) {
        std::cerr << "could not request WebGPU device\n";
        return false;
    }
    context.queue = context.device.getQueue();
    return true;
}

bool Renderer::make_layout() {
    std::array<wgpu::BindGroupLayoutEntry, 6> entries{};
    entries[0].binding = 0;
    entries[0].visibility = wgpu::ShaderStage::Compute;
    entries[0].storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
    entries[0].storageTexture.format = wgpu::TextureFormat::RGBA8Unorm;
    entries[0].storageTexture.viewDimension = wgpu::TextureViewDimension::_2D;
    entries[1].binding = 1;
    entries[1].visibility = wgpu::ShaderStage::Compute;
    entries[1].buffer.type = wgpu::BufferBindingType::Uniform;
    for (std::uint32_t index = 0; index < channel_count; ++index) {
        auto& entry = entries[index + 2U];
        entry.binding = index + 2U;
        entry.visibility = wgpu::ShaderStage::Compute;
        entry.texture.sampleType = wgpu::TextureSampleType::Float;
        entry.texture.viewDimension = wgpu::TextureViewDimension::_2D;
    }
    wgpu::BindGroupLayoutDescriptor descriptor{};
    descriptor.entryCount = entries.size();
    descriptor.entries = entries.data();
    bind_group_layout_ = context_->device.createBindGroupLayout(descriptor);
    WGPUBindGroupLayout raw_layout = bind_group_layout_;
    wgpu::PipelineLayoutDescriptor pipeline_descriptor{};
    pipeline_descriptor.bindGroupLayoutCount = 1;
    pipeline_descriptor.bindGroupLayouts = &raw_layout;
    pipeline_layout_ = context_->device.createPipelineLayout(pipeline_descriptor);
    return bind_group_layout_ != WGPUBindGroupLayout(nullptr) &&
           pipeline_layout_ != WGPUPipelineLayout(nullptr);
}

Renderer::Pass Renderer::make_pass(const std::string& source, bool active) {
    if (!active) {
        return {};
    }
    context_->device.pushErrorScope(wgpu::ErrorFilter::Validation);
    wgpu::ComputePipelineDescriptor descriptor{};
    descriptor.layout = pipeline_layout_;
    descriptor.compute.module = make_shader_module(context_->device, source);
    descriptor.compute.entryPoint = "main";
    Pass pass;
    pass.active = true;
    pass.pipeline = context_->device.createComputePipeline(descriptor);
    bool checked = false;
    bool invalid = false;
    auto callback = context_->device.popErrorScope([&](wgpu::ErrorType type, const char* message) {
        invalid = type != wgpu::ErrorType::NoError;
        if (invalid) std::cerr << (message != nullptr ? message : "shader validation failed") << '\n';
        checked = true;
    });
    while (!checked) context_->device.poll(true);
    (void)callback;
    if (invalid || pass.pipeline == WGPUComputePipeline(nullptr)) {
        pass.active = false;
    }
    return pass;
}

bool Renderer::make_targets(const ChannelSet& channels) {
    for (std::size_t index = 0; index < channel_count; ++index) {
        channel_textures_[index] = make_channel_texture(*context_, channels.image[index]);
        channel_views_[index] = channel_textures_[index].createView();
        for (int side = 0; side < 2; ++side) {
            buffer_textures_[index][side] = make_texture(
                *context_, width_, height_, wgpu::TextureUsage::StorageBinding |
                                                wgpu::TextureUsage::TextureBinding |
                                                wgpu::TextureUsage::CopyDst);
            clear_texture(*context_, buffer_textures_[index][side], width_, height_);
            buffer_views_[index][side] = buffer_textures_[index][side].createView();
        }
    }
    output_texture_ = make_texture(*context_, width_, height_,
                                   wgpu::TextureUsage::StorageBinding |
                                       wgpu::TextureUsage::TextureBinding |
                                       wgpu::TextureUsage::CopySrc);
    output_view_ = output_texture_.createView();
    return output_texture_ != WGPUTexture(nullptr);
}

bool Renderer::init(GpuContext& context, int width, int height, const ShaderSources& sources,
                    const ChannelSet& channels) {
    context_ = &context;
    width_ = width;
    height_ = height;
    if (!make_layout() || !reload(sources)) {
        return false;
    }
    wgpu::BufferDescriptor descriptor{};
    descriptor.size = uniform_buffer_size;
    descriptor.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
    uniform_buffer_ = context.device.createBuffer(descriptor);
    return make_targets(channels);
}

bool Renderer::reload(const ShaderSources& sources) {
    Pass image = make_pass(sources.image, true);
    if (!image.active) {
        return false;
    }
    std::array<Pass, channel_count> buffers;
    for (std::size_t index = 0; index < buffers.size(); ++index) {
        buffers[index] = make_pass(sources.buffers[index], sources.has_buffer[index]);
        if (sources.has_buffer[index] && !buffers[index].active) {
            return false;
        }
    }
    image_pass_ = std::move(image);
    buffer_passes_ = std::move(buffers);
    return true;
}

bool Renderer::dispatch(const Pass& pass, const FrameState& frame, const ChannelSet& channels,
                        const std::array<wgpu::TextureView, channel_count>& views,
                        wgpu::TextureView output) {
    const std::vector<float> uniforms = make_uniforms(frame, channels);
    context_->queue.writeBuffer(uniform_buffer_, 0, uniforms.data(), uniform_payload_size);
    std::array<wgpu::BindGroupEntry, 6> entries{};
    entries[0].binding = 0;
    entries[0].textureView = output;
    entries[1].binding = 1;
    entries[1].buffer = uniform_buffer_;
    entries[1].size = uniform_buffer_size;
    for (std::size_t index = 0; index < views.size(); ++index) {
        entries[index + 2].binding = static_cast<std::uint32_t>(index + 2);
        entries[index + 2].textureView = views[index];
    }
    wgpu::BindGroupDescriptor descriptor{};
    descriptor.layout = bind_group_layout_;
    descriptor.entryCount = entries.size();
    descriptor.entries = entries.data();
    wgpu::BindGroup bind_group = context_->device.createBindGroup(descriptor);
    if (bind_group == WGPUBindGroup(nullptr)) {
        return false;
    }
    wgpu::CommandEncoder encoder = context_->device.createCommandEncoder();
    wgpu::ComputePassEncoder compute = encoder.beginComputePass();
    compute.setPipeline(pass.pipeline);
    compute.setBindGroup(0, bind_group, 0, nullptr);
    compute.dispatchWorkgroups((static_cast<std::uint32_t>(width_) + 7U) / 8U,
                               (static_cast<std::uint32_t>(height_) + 7U) / 8U, 1);
    compute.end();
    context_->queue.submit(encoder.finish());
    return true;
}

bool Renderer::render(const FrameState& frame, const ChannelSet& channels) {
    std::array<wgpu::TextureView, channel_count> previous = channel_views_;
    ChannelSet previous_metadata = channels;
    for (std::size_t index = 0; index < channel_count; ++index) {
        if (buffer_passes_[index].active) {
            previous[index] = buffer_views_[index][buffer_read_index_];
            previous_metadata.image[index].width = width_;
            previous_metadata.image[index].height = height_;
            previous_metadata.image[index].pixels = {0};
        }
    }
    const int write_index = 1 - buffer_read_index_;
    for (std::size_t index = 0; index < channel_count; ++index) {
        if (buffer_passes_[index].active &&
            !dispatch(buffer_passes_[index], frame, previous_metadata, previous,
                      buffer_views_[index][write_index])) {
            return false;
        }
    }
    std::array<wgpu::TextureView, channel_count> current = channel_views_;
    ChannelSet current_metadata = channels;
    for (std::size_t index = 0; index < channel_count; ++index) {
        if (buffer_passes_[index].active) {
            current[index] = buffer_views_[index][write_index];
            current_metadata.image[index].width = width_;
            current_metadata.image[index].height = height_;
            current_metadata.image[index].pixels = {0};
        }
    }
    if (!dispatch(image_pass_, frame, current_metadata, current, output_view_)) {
        return false;
    }
    buffer_read_index_ = write_index;
    return true;
}

bool Renderer::upload_channel(std::size_t index, const ImageChannel& channel) {
    if (index >= channel_count || !channel.loaded()) {
        return false;
    }
    write_texture(*context_, channel_textures_[index], channel);
    return true;
}

wgpu::TextureView Renderer::output_view() const {
    return output_view_;
}

wgpu::Texture Renderer::output_texture() const {
    return output_texture_;
}

std::vector<std::uint8_t> Renderer::read_rgba() {
    constexpr std::uint32_t alignment = 256;
    const std::uint32_t row_size = static_cast<std::uint32_t>(width_) * 4U;
    const std::uint32_t padded_row_size = align_to(row_size, alignment);
    const std::uint64_t size = static_cast<std::uint64_t>(padded_row_size) * height_;
    wgpu::BufferDescriptor descriptor{};
    descriptor.size = size;
    descriptor.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
    wgpu::Buffer buffer = context_->device.createBuffer(descriptor);
    wgpu::ImageCopyTexture source{};
    source.texture = output_texture_;
    wgpu::ImageCopyBuffer destination{};
    destination.buffer = buffer;
    destination.layout.bytesPerRow = padded_row_size;
    destination.layout.rowsPerImage = static_cast<std::uint32_t>(height_);
    wgpu::CommandEncoder encoder = context_->device.createCommandEncoder();
    encoder.copyTextureToBuffer(source, destination,
                                wgpu::Extent3D(static_cast<std::uint32_t>(width_),
                                               static_cast<std::uint32_t>(height_), 1));
    context_->queue.submit(encoder.finish());
    const std::vector<std::uint8_t> padded = map_readback(context_->device, buffer, size);
    if (padded.size() != size) {
        return {};
    }
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width_ * height_) * 4U);
    for (int y = 0; y < height_; ++y) {
        std::memcpy(rgba.data() + static_cast<std::size_t>(y) * row_size,
                    padded.data() + static_cast<std::size_t>(y) * padded_row_size, row_size);
    }
    return rgba;
}

bool write_ppm(const std::string& path, int width, int height,
               const std::vector<std::uint8_t>& rgba) {
    if (rgba.size() != static_cast<std::size_t>(width * height) * 4U) {
        return false;
    }
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << "P6\n" << width << ' ' << height << "\n255\n";
    for (std::size_t pixel = 0; pixel < rgba.size() / 4U; ++pixel) {
        output.write(reinterpret_cast<const char*>(rgba.data() + pixel * 4U), 3);
    }
    return static_cast<bool>(output);
}

} // namespace shady
