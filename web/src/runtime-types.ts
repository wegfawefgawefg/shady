export type ChannelRuntimeSource = {
  audio?: {
    analyser: AnalyserNode;
    duration?: number;
    timeData: Uint8Array<ArrayBuffer>;
    frequencyData: Uint8Array<ArrayBuffer>;
    pixels: Uint8Array<ArrayBuffer>;
    startedAt?: number;
    width: number;
    height: number;
  };
  video?: HTMLVideoElement;
  mirrorCanvas?: HTMLCanvasElement;
  mirrorContext?: CanvasRenderingContext2D;
  mirrored?: boolean;
};

export type BrowserInputState = {
  mouseX: number;
  mouseY: number;
  mouseDown: number;
  mouseClickX: number;
  mouseClickY: number;
  mouseButtons: number[];
  mouseWheelX: number;
  mouseWheelY: number;
  keys: number[];
  gamepadButtons: number[];
  gamepadAxes: number[];
};
