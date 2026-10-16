export type ImageModelConfig = {
    key: string;
    id: string;
    visionDtype: string;
    textDtype: string;
};

export type TextModelConfig = {
    key: string;
    id: string;
    dtype: string;
    maxChars: number;
};

export type AIModelPreset = {
    image: ImageModelConfig;
    text: TextModelConfig;
    caption: { task: string; visionDtype: string };
    ocr: 'tiny' | 'small';
    keepAliveMs: number;
    videoFrames: number;
};

export type AIConfig = {
    preset: AIModelPreset;
    threads: number;
    cacheDir: string;
};

export type PrivacyEntity = {
    entity_group: string;
    score: number;
    word: string;
};

export type AIMethods = {
    warmUp: () => Promise<void>;
    unload: () => Promise<void>;
    embedImage: (path: string) => Promise<number[]>;
    captionImage: (path: string) => Promise<string>;
    embedImageQuery: (query: string) => Promise<number[]>;
    embedVideo: (path: string, duration?: number) => Promise<number[]>;
    embedText: (text: string, query?: boolean) => Promise<number[] | null>;
    recognizeText: (image: string | Uint8Array) => Promise<string>;
    findPrivacyEntities: (text: string) => Promise<PrivacyEntity[]>;
};

export type AIRequest = {
    id: number;
    method: keyof AIMethods;
    config: AIConfig;
    args: unknown[];
};

export type AIResponse =
    | { type: 'result'; id: number; value: unknown }
    | { type: 'error'; id: number; message: string }
    | { type: 'log'; message: string };
