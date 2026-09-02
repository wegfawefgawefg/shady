import { describe, expect, test } from "vitest";
import { normalizeProject } from "./project";

const shader = "fn shade(pixel: ShadyPixel) -> vec4<f32> { return vec4<f32>(pixel.uv, 0.0, 1.0); }";

describe("project bundles", () => {
  test("normalizes missing channels and buffers", () => {
    const project = normalizeProject({
      files: [{ path: "main.wgsl", content: shader }],
      settings: { main: "main.wgsl", size: "gba", scale: 4, channels: [] }
    });

    expect(project.settings.channels).toHaveLength(4);
    expect(project.settings.channels[0]).toMatchObject({ kind: "fallback", name: "channel0", width: 1, height: 1 });
    expect(project.settings.buffers).toEqual([null, null, null, null]);
  });

  test("preserves direct WGSL feedback passes", () => {
    const project = normalizeProject({
      files: [
        { path: "main.wgsl", content: shader },
        { path: "buffer.wgsl", content: shader }
      ],
      settings: {
        main: "main.wgsl",
        size: "gba",
        scale: 4,
        channels: [],
        buffers: [{ file: "buffer.wgsl" }]
      }
    });

    expect(project.settings.buffers).toHaveLength(4);
    expect(project.settings.buffers?.[0]).toEqual({ file: "buffer.wgsl" });
    expect(project.settings.buffers?.[1]).toBeNull();
  });

  test.each([
    { kind: "noise" as const, name: "noise:clouds", width: 256, height: 256, seed: "clouds" },
    { kind: "webcam" as const, name: "webcam", width: 640, height: 480 },
    { kind: "microphone" as const, name: "microphone", width: 512, height: 2, sampleRate: 48000 },
    { kind: "video" as const, name: "clip.mp4", width: 640, height: 360 },
    { kind: "audio" as const, name: "loop.wav", width: 512, height: 2, sampleRate: 44100 }
  ])("preserves $kind channel metadata", (channel) => {
    const project = normalizeProject({
      files: [{ path: "main.wgsl", content: shader }],
      settings: { main: "main.wgsl", size: "gba", scale: 4, channels: [channel] }
    });
    expect(project.settings.channels[0]).toMatchObject(channel);
  });

  test("preserves URL-backed media metadata", () => {
    const channel = {
      kind: "video" as const,
      name: "remote clip",
      width: 640,
      height: 360,
      sourceUrl: "https://example.com/clip.mp4"
    };
    const project = normalizeProject({
      files: [{ path: "main.wgsl", content: shader }],
      settings: { main: "main.wgsl", size: "gba", scale: 4, channels: [channel] }
    });
    expect(project.settings.channels[0]).toMatchObject(channel);
  });
});
