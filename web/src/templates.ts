import { normalizeProject, type ProjectBundle, type ProjectFile } from "./project";

import audioScope from "../../examples/audio/scope.wgsl?raw";
import gradient from "../../examples/basics/gradient.wgsl?raw";
import plasma from "../../examples/basics/plasma.wgsl?raw";
import trails from "../../examples/buffers/trails.wgsl?raw";
import trailsBuffer from "../../examples/buffers/trails_buffer.wgsl?raw";
import liveControls from "../../examples/input/live_controls.wgsl?raw";
import frostedGlass from "../../examples/pills/frosted_glass.wgsl?raw";
import sphere from "../../examples/raymarch/sphere.wgsl?raw";
import image from "../../examples/textures/image.wgsl?raw";
import noise from "../../examples/textures/noise.wgsl?raw";
import videoEdges from "../../examples/video/edges.wgsl?raw";

export type TemplateProject = {
  id: string;
  name: string;
  files: ProjectFile[];
  main: string;
  size?: ProjectBundle["settings"]["size"];
  scale?: number;
  channels?: ProjectBundle["settings"]["channels"];
  buffers?: ProjectBundle["settings"]["buffers"];
};

const fallbackChannels: ProjectBundle["settings"]["channels"] = [
  { kind: "fallback", name: "channel0", width: 1, height: 1 },
  { kind: "fallback", name: "channel1", width: 1, height: 1 },
  { kind: "fallback", name: "channel2", width: 1, height: 1 },
  { kind: "fallback", name: "channel3", width: 1, height: 1 }
];

const checkerChannel: ProjectBundle["settings"]["channels"][number] = {
  kind: "image",
  name: "checker.png",
  width: 8,
  height: 8,
  sourceUrl: "examples/assets/checker.png"
};

const twoToneChannel: ProjectBundle["settings"]["channels"][number] = {
  kind: "audio",
  name: "two_tone.wav",
  width: 512,
  height: 2,
  sampleRate: 44100,
  sourceUrl: "examples/assets/audio/two_tone.wav"
};

const testVideoChannel: ProjectBundle["settings"]["channels"][number] = {
  kind: "video",
  name: "testsrc_160x90.mp4",
  width: 160,
  height: 90,
  sourceUrl: "examples/assets/video/testsrc_160x90.mp4"
};

function withChannels(channels: ProjectBundle["settings"]["channels"]): ProjectBundle["settings"]["channels"] {
  return [
    ...channels.map((channel) => ({ ...channel })),
    ...fallbackChannels.slice(channels.length).map((channel) => ({ ...channel }))
  ];
}

function single(path: string, content: string, extras: Partial<TemplateProject> = {}): TemplateProject {
  return {
    id: path,
    name: path.replace("examples/", "").replace(".wgsl", ""),
    files: [{ path, content }],
    main: path,
    size: "gba",
    scale: 4,
    ...extras
  };
}

export const templateProjects: TemplateProject[] = [
  single("examples/pills/frosted_glass.wgsl", frostedGlass, { size: "640x420", scale: 1 }),
  single("examples/basics/gradient.wgsl", gradient),
  single("examples/basics/plasma.wgsl", plasma),
  single("examples/input/live_controls.wgsl", liveControls),
  {
    id: "examples/buffers/trails.wgsl",
    name: "buffers/trails",
    files: [
      { path: "examples/buffers/trails.wgsl", content: trails },
      { path: "examples/buffers/trails_buffer.wgsl", content: trailsBuffer }
    ],
    main: "examples/buffers/trails.wgsl",
    size: "gba",
    scale: 4,
    buffers: [{ file: "examples/buffers/trails_buffer.wgsl" }, null, null, null]
  },
  single("examples/raymarch/sphere.wgsl", sphere, { size: "320x320", scale: 2 }),
  single("examples/textures/image.wgsl", image, { channels: withChannels([checkerChannel]) }),
  single("examples/textures/noise.wgsl", noise, {
    channels: withChannels([{ kind: "noise", name: "noise:42", width: 256, height: 256, seed: "42" }])
  }),
  single("examples/audio/scope.wgsl", audioScope, { channels: withChannels([twoToneChannel]) }),
  single("examples/video/edges.wgsl", videoEdges, {
    size: "320x180",
    scale: 2,
    channels: withChannels([testVideoChannel])
  })
];

export function makeTemplateProject(template: TemplateProject): ProjectBundle {
  return normalizeProject({
    files: template.files.map((file) => ({ ...file })),
    settings: {
      main: template.main,
      size: template.size ?? "gba",
      scale: template.scale ?? 4,
      channels: template.channels ?? fallbackChannels.map((channel) => ({ ...channel })),
      buffers: template.buffers ?? [null, null, null, null]
    }
  });
}
