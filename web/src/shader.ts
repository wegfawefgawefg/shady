import entry from "../../runtime/entry.wgsl?raw";
import prelude from "../../runtime/prelude.wgsl?raw";

export const preludeLineCount = prelude.split("\n").length;

export function composeShader(source: string): string {
  if (!/\bfn\s+shade\s*\(/.test(source)) {
    throw new Error("The shader must define fn shade(pixel: ShadyPixel) -> vec4<f32>.");
  }
  return `${prelude}\n\n${source.trim()}\n\n${entry}`;
}

export function projectFileSource(files: ReadonlyArray<{ path: string; content: string }>, path: string): string {
  const file = files.find((candidate) => candidate.path === path);
  if (!file) {
    throw new Error(`Project file not found: ${path}`);
  }
  return file.content;
}
