import { HighlightStyle, StreamLanguage, syntaxHighlighting } from "@codemirror/language";
import { tags } from "@lezer/highlight";
import { EditorView, basicSetup } from "codemirror";

const keywords = new Set([
  "alias",
  "array",
  "binding",
  "bool",
  "break",
  "builtin",
  "case",
  "compute",
  "const",
  "continue",
  "default",
  "else",
  "false",
  "f32",
  "fn",
  "for",
  "fragment",
  "if",
  "i32",
  "let",
  "location",
  "loop",
  "override",
  "return",
  "select",
  "storage",
  "struct",
  "switch",
  "true",
  "u32",
  "uniform",
  "var",
  "vec2",
  "vec3",
  "vec4",
  "vertex",
  "workgroup",
  "workgroup_size"
]);

const builtins = new Set([
  "abs",
  "ceil",
  "clamp",
  "cos",
  "distance",
  "dot",
  "floor",
  "fract",
  "length",
  "max",
  "min",
  "mix",
  "normalize",
  "pow",
  "round",
  "sin",
  "smoothstep",
  "sqrt",
  "step",
  "textureLoad",
  "textureStore"
]);

const language = StreamLanguage.define({
  token(stream) {
    if (stream.eatSpace()) {
      return null;
    }
    if (stream.match("//")) {
      stream.skipToEnd();
      return "comment";
    }
    if (stream.match(/@[A-Za-z_][A-Za-z0-9_]*/)) {
      return "attribute";
    }
    if (stream.match(/-?(?:\d+\.\d*|\.\d+|\d+)(?:e[+-]?\d+)?/i)) {
      return "number";
    }
    const match = stream.match(/[A-Za-z_][A-Za-z0-9_]*/, false);
    if (match && match !== true) {
      const token = match[0];
      stream.match(/[A-Za-z_][A-Za-z0-9_]*/);
      if (keywords.has(token)) {
        return "keyword";
      }
      if (builtins.has(token)) {
        return "operatorKeyword";
      }
      return "name";
    }
    stream.next();
    return null;
  }
});

const highlight = HighlightStyle.define([
  { tag: tags.comment, color: "#6f7a85", fontStyle: "italic" },
  { tag: tags.keyword, color: "#ffcb6b" },
  { tag: tags.operatorKeyword, color: "#82aaff" },
  { tag: tags.number, color: "#f78c6c" },
  { tag: tags.string, color: "#c3e88d" },
  { tag: tags.atom, color: "#89ddff" },
  { tag: tags.variableName, color: "#d7ff65" },
  { tag: tags.name, color: "#e6e8ea" }
]);

export function createEditor(parent: HTMLElement, onChange: () => void): EditorView {
  return new EditorView({
    parent,
    doc: "",
    extensions: [
      basicSetup,
      language,
      syntaxHighlighting(highlight),
      EditorView.updateListener.of((update) => {
        if (update.docChanged) {
          onChange();
        }
      }),
      EditorView.theme({
        "&": {
          height: "100%",
          minHeight: "0",
          backgroundColor: "#101317",
          color: "#e6e8ea",
          fontSize: "13px"
        },
        ".cm-scroller": {
          fontFamily: '"SFMono-Regular", Consolas, "Liberation Mono", monospace',
          lineHeight: "1.45"
        },
        ".cm-content": { padding: "12px" },
        ".cm-gutters": {
          backgroundColor: "#101317",
          color: "#59626d",
          borderRight: "1px solid #2b3138"
        },
        ".cm-activeLine, .cm-activeLineGutter": { backgroundColor: "#171d23" },
        ".cm-selectionBackground": { backgroundColor: "#31475f !important" }
      })
    ]
  });
}

export function setEditorText(view: EditorView, text: string): void {
  view.dispatch({ changes: { from: 0, to: view.state.doc.length, insert: text } });
}
