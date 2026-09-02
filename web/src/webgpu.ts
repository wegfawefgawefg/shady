import { makeNoisePixels, noiseTextureSize } from "./noise";
import presentShader from "./present.wgsl?raw";
import { parseSize, type ChannelSetting, type ProjectFile, type ProjectSettings } from "./project";
import type { BrowserInputState, ChannelRuntimeSource } from "./runtime-types";
import { composeShader, projectFileSource } from "./shader";
import { uniformByteSize, writeUniforms } from "./uniforms";

export type { BrowserInputState, ChannelRuntimeSource } from "./runtime-types";

export type GpuContext = {
  device: GPUDevice;
  adapterLabel: string;
  canvasContext: GPUCanvasContext;
  format: GPUTextureFormat;
  sampler: GPUSampler;
  renderPipeline: GPURenderPipeline;
  computeBindGroupLayout: GPUBindGroupLayout;
  computePipelineLayout: GPUPipelineLayout;
  errors: string[];
};

export type ProgramState = {
  source: string;
  sizeKey: string;
  computePipeline: GPUComputePipeline;
  uniformBuffer: GPUBuffer;
  outputTexture: GPUTexture;
  outputView: GPUTextureView;
  renderBindGroup: GPUBindGroup;
  channelTextures: GPUTexture[];
  channelViews: GPUTextureView[];
  channelMetadata: ChannelSetting[];
  bufferPasses: BufferPassState[];
  imageBindGroups: [GPUBindGroup, GPUBindGroup];
  bufferReadIndex: number;
};

type BufferPassState = {
  channel: number;
  source: string;
  computePipeline: GPUComputePipeline;
  textures: [GPUTexture, GPUTexture];
  views: [GPUTextureView, GPUTextureView];
  bindGroups: [GPUBindGroup, GPUBindGroup];
};

type PendingBufferPassState = Omit<BufferPassState, "bindGroups">;

export function configureCanvas(context: Pick<GpuContext, "canvasContext" | "device" | "format">): void {
  context.canvasContext.configure({
    device: context.device,
    format: context.format,
    alphaMode: "opaque"
  });
}

export async function initWebGpu(canvas: HTMLCanvasElement): Promise<GpuContext> {
  if (!navigator.gpu) {
    throw new Error("WebGPU is not available in this browser.");
  }
  const attempts: Array<GPURequestAdapterOptions & { label: string }> = [
    { label: "default" },
    { label: "high-performance", powerPreference: "high-performance" },
    { label: "low-power", powerPreference: "low-power" }
  ];
  let adapter: GPUAdapter | null = null;
  let adapterLabel = "";
  for (const attempt of attempts) {
    adapter = await navigator.gpu.requestAdapter(attempt);
    if (adapter) {
      adapterLabel = attempt.label;
      break;
    }
  }
  if (!adapter) {
    throw new Error(
      "No WebGPU adapter is available. On Linux Chrome/Brave, try launching with --enable-unsafe-webgpu --enable-features=Vulkan --ignore-gpu-blocklist and check chrome://gpu."
    );
  }
  const device = await adapter.requestDevice();
  const errors: string[] = [];
  device.addEventListener("uncapturederror", (event) => {
    const message = event.error.message;
    errors.push(message);
    console.error(message);
  });
  const canvasContext = canvas.getContext("webgpu");
  if (!canvasContext) {
    throw new Error("Could not create WebGPU canvas context.");
  }
  const format = navigator.gpu.getPreferredCanvasFormat();

  const sampler = device.createSampler({
    magFilter: "nearest",
    minFilter: "nearest",
    mipmapFilter: "nearest"
  });
  const computeBindGroupLayout = device.createBindGroupLayout({
    entries: [
      {
        binding: 0,
        visibility: GPUShaderStage.COMPUTE,
        storageTexture: { access: "write-only", format: "rgba8unorm", viewDimension: "2d" }
      },
      {
        binding: 1,
        visibility: GPUShaderStage.COMPUTE,
        buffer: { type: "uniform" }
      },
      ...[2, 3, 4, 5].map((binding) => ({
        binding,
        visibility: GPUShaderStage.COMPUTE,
        texture: { sampleType: "float" as GPUTextureSampleType, viewDimension: "2d" as GPUTextureViewDimension }
      }))
    ]
  });
  const computePipelineLayout = device.createPipelineLayout({ bindGroupLayouts: [computeBindGroupLayout] });

  const renderModule = device.createShaderModule({ code: presentShader });
  const renderPipeline = device.createRenderPipeline({
    layout: "auto",
    vertex: { module: renderModule, entryPoint: "vs" },
    fragment: { module: renderModule, entryPoint: "fs", targets: [{ format }] },
    primitive: { topology: "triangle-list" }
  });

  const context = {
    device,
    adapterLabel,
    canvasContext,
    format,
    sampler,
    renderPipeline,
    computeBindGroupLayout,
    computePipelineLayout,
    errors
  };
  configureCanvas(context);
  return context;
}

function makeFallbackTexture(device: GPUDevice): GPUTexture {
  const texture = device.createTexture({
    size: { width: 1, height: 1 },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST
  });
  device.queue.writeTexture({ texture }, new Uint8Array([255, 255, 255, 255]), {}, { width: 1, height: 1 });
  return texture;
}

function makeNoiseTexture(device: GPUDevice, seed: string): GPUTexture {
  const texture = device.createTexture({
    size: { width: noiseTextureSize, height: noiseTextureSize },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST
  });
  device.queue.writeTexture(
    { texture },
    makeNoisePixels(seed),
    { bytesPerRow: noiseTextureSize * 4, rowsPerImage: noiseTextureSize },
    { width: noiseTextureSize, height: noiseTextureSize }
  );
  return texture;
}

async function loadImageBitmap(src: string): Promise<ImageBitmap> {
  const response = await fetch(src);
  const blob = await response.blob();
  return createImageBitmap(blob);
}

async function makeChannelTexture(device: GPUDevice, channel: ChannelSetting): Promise<GPUTexture> {
  if (channel.kind === "noise" || channel.seed) {
    return makeNoiseTexture(device, channel.seed ?? channel.name);
  }
  const imageSource = channel.imageDataUrl ?? channel.sourceUrl;
  if (!imageSource) {
    return makeFallbackTexture(device);
  }
  const image = await loadImageBitmap(imageSource);
  const texture = device.createTexture({
    size: { width: image.width, height: image.height },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST | GPUTextureUsage.RENDER_ATTACHMENT
  });
  device.queue.copyExternalImageToTexture({ source: image }, { texture }, { width: image.width, height: image.height });
  image.close();
  return texture;
}

function makeVideoTexture(device: GPUDevice, video: HTMLVideoElement): GPUTexture {
  const width = Math.max(1, video.videoWidth);
  const height = Math.max(1, video.videoHeight);
  return device.createTexture({
    size: { width, height },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST | GPUTextureUsage.RENDER_ATTACHMENT
  });
}

function makeLiveDataTexture(device: GPUDevice, width: number, height: number): GPUTexture {
  return device.createTexture({
    size: { width: Math.max(1, width), height: Math.max(1, height) },
    format: "rgba8unorm",
    usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST
  });
}

function makeOutputTexture(device: GPUDevice, width: number, height: number): GPUTexture {
  return device.createTexture({
    size: { width, height },
    format: "rgba8unorm",
    usage:
      GPUTextureUsage.STORAGE_BINDING |
      GPUTextureUsage.TEXTURE_BINDING |
      GPUTextureUsage.COPY_SRC |
      GPUTextureUsage.RENDER_ATTACHMENT
  });
}

function clearTextures(device: GPUDevice, textures: GPUTexture[]): void {
  if (textures.length === 0) return;
  const encoder = device.createCommandEncoder();
  for (const texture of textures) {
    const pass = encoder.beginRenderPass({
      colorAttachments: [{
        view: texture.createView(),
        clearValue: { r: 0, g: 0, b: 0, a: 0 },
        loadOp: "clear",
        storeOp: "store"
      }]
    });
    pass.end();
  }
  device.queue.submit([encoder.finish()]);
}

async function createComputePipeline(
  device: GPUDevice,
  layout: GPUPipelineLayout,
  source: string,
  label: string
): Promise<GPUComputePipeline> {
  device.pushErrorScope("validation");
  try {
    const module = device.createShaderModule({ code: source, label: `${label} shader` });
    const pipeline = await device.createComputePipelineAsync({
      label,
      layout,
      compute: { module, entryPoint: "main" }
    });
    const error = await device.popErrorScope();
    if (error) {
      throw new Error(error.message);
    }
    return pipeline;
  } catch (error) {
    const scopedError = await device.popErrorScope().catch(() => null);
    if (scopedError) {
      throw new Error(scopedError.message);
    }
    throw error;
  }
}

export async function createProgram(
  context: GpuContext,
  source: string,
  settings: ProjectSettings,
  files: ReadonlyArray<ProjectFile>,
  channelSources: ReadonlyMap<number, ChannelRuntimeSource> = new Map()
): Promise<ProgramState> {
  const { device } = context;
  const size = parseSize(settings.size);
  const computePipeline = await createComputePipeline(device, context.computePipelineLayout, composeShader(source), "image pass");
  const outputTexture = makeOutputTexture(device, size.width, size.height);
  const outputView = outputTexture.createView();
  const uniformBuffer = device.createBuffer({
    size: uniformByteSize,
    usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST
  });
  const normalizedChannels = [...(settings.channels ?? [])];
  while (normalizedChannels.length < 4) {
    normalizedChannels.push({ kind: "fallback", name: `channel${normalizedChannels.length}`, width: 1, height: 1 });
  }
  const channelTextures = await Promise.all(
    normalizedChannels.slice(0, 4).map((channel, index) => {
      const source = channelSources.get(index);
      const audio = source?.audio;
      if (audio) {
        return makeLiveDataTexture(device, audio.width, audio.height);
      }
      const video = source?.video;
      if (video) {
        return makeVideoTexture(device, video);
      }
      return makeChannelTexture(device, channel);
    })
  );
  const pendingBufferPasses = await Promise.all(
    (settings.buffers ?? []).slice(0, 4).map(async (buffer, channel): Promise<PendingBufferPassState | null> => {
      if (!buffer?.file) {
        return null;
      }
      const bufferSource = projectFileSource(files, buffer.file);
      const bufferPipeline = await createComputePipeline(
        device,
        context.computePipelineLayout,
        composeShader(bufferSource),
        `buffer${channel} pass`
      );
      const first = makeOutputTexture(device, size.width, size.height);
      const second = makeOutputTexture(device, size.width, size.height);
      return {
        channel,
        source: bufferSource,
        computePipeline: bufferPipeline,
        textures: [first, second],
        views: [first.createView(), second.createView()]
      };
    })
  );
  const channelViews = channelTextures.map((texture) => texture.createView());
  const pendingBuffers = pendingBufferPasses.filter((buffer): buffer is PendingBufferPassState => buffer !== null);
  clearTextures(device, pendingBuffers.flatMap((buffer) => buffer.textures));
  const previousChannelViews = (readIndex: number): GPUTextureView[] =>
    channelViews.map((view, channel) => pendingBuffers.find((buffer) => buffer.channel === channel)?.views[readIndex] ?? view);
  const currentChannelViews = (readIndex: number): GPUTextureView[] =>
    channelViews.map((view, channel) => pendingBuffers.find((buffer) => buffer.channel === channel)?.views[1 - readIndex] ?? view);
  const bufferPasses: BufferPassState[] = pendingBuffers.map((buffer) => ({
    ...buffer,
    bindGroups: [0, 1].map((readIndex) =>
      device.createBindGroup({
        layout: context.computeBindGroupLayout,
        entries: [
          { binding: 0, resource: buffer.views[1 - readIndex] },
          { binding: 1, resource: { buffer: uniformBuffer } },
          ...previousChannelViews(readIndex).map((view, index) => ({ binding: index + 2, resource: view }))
        ]
      })
    ) as [GPUBindGroup, GPUBindGroup]
  }));
  const imageBindGroups = [0, 1].map((readIndex) =>
    device.createBindGroup({
      layout: context.computeBindGroupLayout,
      entries: [
        { binding: 0, resource: outputView },
        { binding: 1, resource: { buffer: uniformBuffer } },
        ...currentChannelViews(readIndex).map((view, index) => ({ binding: index + 2, resource: view }))
      ]
    })
  ) as [GPUBindGroup, GPUBindGroup];
  const renderBindGroup = device.createBindGroup({
    layout: context.renderPipeline.getBindGroupLayout(0),
    entries: [
      { binding: 0, resource: context.sampler },
      { binding: 1, resource: outputView }
    ]
  });
  return {
    source,
    sizeKey: `${size.width}x${size.height}`,
    computePipeline,
    uniformBuffer,
    outputTexture,
    outputView,
    renderBindGroup,
    channelTextures,
    channelViews,
    channelMetadata: normalizedChannels.slice(0, 4),
    bufferPasses,
    imageBindGroups,
    bufferReadIndex: 0
  };
}

export function destroyProgram(program: ProgramState | null): void {
  if (!program) {
    return;
  }
  program.outputTexture.destroy();
  program.uniformBuffer.destroy();
  for (const texture of program.channelTextures) {
    texture.destroy();
  }
  for (const buffer of program.bufferPasses) {
    buffer.textures[0].destroy();
    buffer.textures[1].destroy();
  }
}

function updateLiveChannelTextures(
  context: GpuContext,
  program: ProgramState,
  channelSources: ReadonlyMap<number, ChannelRuntimeSource>
): void {
  for (let channel = 0; channel < program.channelTextures.length; ++channel) {
    const source = channelSources.get(channel);
    if (source?.audio) {
      const { analyser, timeData, frequencyData, pixels, width, height } = source.audio;
      analyser.getByteTimeDomainData(timeData);
      analyser.getByteFrequencyData(frequencyData);
      let offset = 0;
      for (let x = 0; x < width; ++x) {
        const value = timeData[x] ?? 128;
        pixels[offset++] = value;
        pixels[offset++] = value;
        pixels[offset++] = value;
        pixels[offset++] = 255;
      }
      for (let x = 0; x < width; ++x) {
        const value = frequencyData[x] ?? 0;
        pixels[offset++] = value;
        pixels[offset++] = value;
        pixels[offset++] = value;
        pixels[offset++] = 255;
      }
      context.device.queue.writeTexture(
        { texture: program.channelTextures[channel] },
        pixels,
        { bytesPerRow: width * 4, rowsPerImage: height },
        { width, height }
      );
      continue;
    }
    const video = source?.video;
    if (!source || !video || video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA) {
      continue;
    }
    const width = Math.max(1, video.videoWidth);
    const height = Math.max(1, video.videoHeight);
    if (source.mirrored) {
      if (!source.mirrorCanvas || !source.mirrorContext) {
        source.mirrorCanvas = document.createElement("canvas");
        source.mirrorContext = source.mirrorCanvas.getContext("2d") ?? undefined;
      }
      if (!source.mirrorContext) {
        continue;
      }
      source.mirrorCanvas.width = width;
      source.mirrorCanvas.height = height;
      source.mirrorContext.setTransform(-1, 0, 0, 1, width, 0);
      source.mirrorContext.drawImage(video, 0, 0, width, height);
      source.mirrorContext.setTransform(1, 0, 0, 1, 0, 0);
      context.device.queue.copyExternalImageToTexture(
        { source: source.mirrorCanvas },
        { texture: program.channelTextures[channel] },
        { width, height }
      );
      continue;
    }
    context.device.queue.copyExternalImageToTexture(
      { source: video },
      { texture: program.channelTextures[channel] },
      { width, height }
    );
  }
}

export function renderFrame(
  context: GpuContext,
  program: ProgramState,
  settings: ProjectSettings,
  frame: number,
  start: number,
  channelSources: ReadonlyMap<number, ChannelRuntimeSource> = new Map(),
  inputState?: BrowserInputState
): void {
  const { device } = context;
  const size = parseSize(settings.size);
  updateLiveChannelTextures(context, program, channelSources);
  writeUniforms(device, program.uniformBuffer, settings, frame, start, channelSources, inputState);

  const readIndex = program.bufferReadIndex;
  const writeIndex = 1 - readIndex;

  const encoder = device.createCommandEncoder();
  for (const buffer of program.bufferPasses) {
    const pass = encoder.beginComputePass();
    pass.setPipeline(buffer.computePipeline);
    pass.setBindGroup(0, buffer.bindGroups[readIndex]);
    pass.dispatchWorkgroups(Math.ceil(size.width / 8), Math.ceil(size.height / 8));
    pass.end();
  }
  const computePass = encoder.beginComputePass();
  computePass.setPipeline(program.computePipeline);
  computePass.setBindGroup(0, program.imageBindGroups[readIndex]);
  computePass.dispatchWorkgroups(Math.ceil(size.width / 8), Math.ceil(size.height / 8));
  computePass.end();

  const renderPass = encoder.beginRenderPass({
    colorAttachments: [
      {
        view: context.canvasContext.getCurrentTexture().createView(),
        clearValue: { r: 0, g: 0, b: 0, a: 1 },
        loadOp: "clear",
        storeOp: "store"
      }
    ]
  });
  renderPass.setPipeline(context.renderPipeline);
  renderPass.setBindGroup(0, program.renderBindGroup);
  renderPass.draw(6);
  renderPass.end();

  device.queue.submit([encoder.finish()]);
  if (program.bufferPasses.length > 0) {
    program.bufferReadIndex = writeIndex;
  }
}
