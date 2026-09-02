import { parseSize, type ProjectSettings } from "./project";
import type { BrowserInputState, ChannelRuntimeSource } from "./runtime-types";

const channelOffset = 16;
const floatCount = channelOffset + 16 + 512 + 8 + 4 + 32 + 16;
const lastFrameTimes = new WeakMap<GPUBuffer, number>();

export const uniformByteSize = floatCount * 4;

export function writeUniforms(
  device: GPUDevice,
  buffer: GPUBuffer,
  settings: ProjectSettings,
  frame: number,
  start: number,
  channelSources: ReadonlyMap<number, ChannelRuntimeSource>,
  inputState?: BrowserInputState
): void {
  const size = parseSize(settings.size);
  const values = new Float32Array(floatCount);
  const now = performance.now() / 1000;
  const previous = lastFrameTimes.get(buffer) ?? now - 1 / 60;
  lastFrameTimes.set(buffer, now);
  const date = new Date();
  values[0] = now - start;
  values[1] = Math.max(0, now - previous);
  values[2] = frame;
  values[3] = size.width;
  values[4] = size.height;
  values[5] = inputState?.mouseX ?? 0;
  values[6] = inputState?.mouseY ?? 0;
  values[7] = inputState?.mouseDown ?? 0;
  values[8] = inputState?.mouseClickX ?? 0;
  values[9] = inputState?.mouseClickY ?? 0;
  values[10] = date.getHours() * 3600 + date.getMinutes() * 60 + date.getSeconds() + date.getMilliseconds() / 1000;
  values[11] = date.getFullYear();
  values[12] = date.getMonth() + 1;
  values[13] = date.getDate();

  for (let channel = 0; channel < 4; ++channel) {
    const metadata = settings.channels[channel] ?? { width: 1, height: 1 };
    const source = channelSources.get(channel);
    const bufferPass = settings.buffers?.[channel];
    const offset = channelOffset + channel * 4;
    values[offset] = bufferPass ? size.width : metadata.width || 1;
    values[offset + 1] = bufferPass ? size.height : metadata.height || 1;
    values[offset + 2] = metadata.kind === "video" && source?.video
      ? source.video.currentTime
      : metadata.kind === "audio" && source?.audio?.startedAt !== undefined
        ? source.audio.duration
          ? (now - source.audio.startedAt) % source.audio.duration
          : now - source.audio.startedAt
        : metadata.kind === "webcam" || metadata.kind === "microphone" ? now - start : 0;
    values[offset + 3] = metadata.sampleRate ?? 0;
  }

  const keyOffset = channelOffset + 16;
  for (let index = 0; index < Math.min(512, inputState?.keys.length ?? 0); ++index)
    values[keyOffset + index] = inputState?.keys[index] ?? 0;
  const mouseOffset = keyOffset + 512;
  for (let index = 0; index < Math.min(8, inputState?.mouseButtons.length ?? 0); ++index)
    values[mouseOffset + index] = inputState?.mouseButtons[index] ?? 0;
  values[mouseOffset + 8] = inputState?.mouseWheelX ?? 0;
  values[mouseOffset + 9] = inputState?.mouseWheelY ?? 0;
  const gamepadButtonOffset = mouseOffset + 12;
  for (let index = 0; index < Math.min(32, inputState?.gamepadButtons.length ?? 0); ++index)
    values[gamepadButtonOffset + index] = inputState?.gamepadButtons[index] ?? 0;
  const gamepadAxisOffset = gamepadButtonOffset + 32;
  for (let index = 0; index < Math.min(16, inputState?.gamepadAxes.length ?? 0); ++index)
    values[gamepadAxisOffset + index] = inputState?.gamepadAxes[index] ?? 0;
  device.queue.writeBuffer(buffer, 0, values);
}
