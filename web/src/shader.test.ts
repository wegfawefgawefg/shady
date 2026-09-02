import { describe, expect, it } from "vitest";
import { composeShader, projectFileSource } from "./shader";

describe("shader composition", () => {
  it("wraps one direct WGSL shade function", () => {
    const source = "fn shade(pixel: ShadyPixel) -> vec4<f32> { return vec4<f32>(pixel.uv, 0.0, 1.0); }";
    const composed = composeShader(source);
    expect(composed).toContain("struct ShadyInputs");
    expect(composed).toContain(source);
    expect(composed).toContain("@compute @workgroup_size(8, 8, 1)");
  });

  it("rejects a source without the public entry function", () => {
    expect(() => composeShader("fn nope() {}" )).toThrow(/fn shade/);
  });

  it("finds a named project file", () => {
    expect(projectFileSource([{ path: "main.wgsl", content: "shader" }], "main.wgsl")).toBe("shader");
  });
});
