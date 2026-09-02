import { describe, expect, test } from "vitest";
import { makeTemplateProject, templateProjects } from "./templates";

describe("template projects", () => {
  test("includes direct WGSL examples for every runtime path", () => {
    expect(templateProjects.map((template) => template.id)).toEqual([
      "examples/pills/frosted_glass.wgsl",
      "examples/basics/gradient.wgsl",
      "examples/basics/plasma.wgsl",
      "examples/input/live_controls.wgsl",
      "examples/buffers/trails.wgsl",
      "examples/raymarch/sphere.wgsl",
      "examples/textures/image.wgsl",
      "examples/textures/noise.wgsl",
      "examples/audio/scope.wgsl",
      "examples/video/edges.wgsl"
    ]);
  });

  test("builds the feedback project without generated source", () => {
    const project = makeTemplateProject(
      templateProjects.find((template) => template.id === "examples/buffers/trails.wgsl")!
    );
    expect(project.files.map((file) => file.path)).toContain("examples/buffers/trails_buffer.wgsl");
    expect(project.settings.buffers?.[0]).toEqual({ file: "examples/buffers/trails_buffer.wgsl" });
  });

  test("starts with the motivating frosted pill study", () => {
    const project = makeTemplateProject(templateProjects[0]);
    expect(project.settings.main).toBe("examples/pills/frosted_glass.wgsl");
    expect(project.settings.size).toBe("640x420");
    expect(project.files[0].content).toContain("capsule_distance");
  });

  test("preloads image, audio, video, and noise channels", () => {
    const byId = (id: string) => makeTemplateProject(templateProjects.find((template) => template.id === id)!);
    expect(byId("examples/textures/image.wgsl").settings.channels[0]).toMatchObject({ kind: "image", name: "checker.png" });
    expect(byId("examples/textures/noise.wgsl").settings.channels[0]).toMatchObject({ kind: "noise", seed: "42" });
    expect(byId("examples/audio/scope.wgsl").settings.channels[0]).toMatchObject({ kind: "audio", name: "two_tone.wav" });
    expect(byId("examples/video/edges.wgsl").settings.channels[0]).toMatchObject({ kind: "video", name: "testsrc_160x90.mp4" });
  });
});
