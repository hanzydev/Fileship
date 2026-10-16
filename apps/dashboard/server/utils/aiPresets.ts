export type AIPresetName = 'low' | 'medium' | 'high';

export type SearchCutoff = { floor: number; ratio: number };

export type ImageModel = {
    key: string;
    id: string;
    visionDtype: string;
    textDtype: string;
    cutoff: SearchCutoff;
};

export type TextModel = {
    key: string;
    id: string;
    dtype: string;
    maxChars: number;
    cutoff: SearchCutoff;
};

export type AIPreset = {
    image: ImageModel;
    text: TextModel;
    caption: { task: string; visionDtype: string };
    ocr: 'tiny' | 'small';
    keepAliveMs: number;
    videoFrames: number;
};

const MSIGLIP = {
    key: 'msiglip-b16',
    id: 'pulsejet/siglip-base-patch16-256-multilingual-onnx',
    visionDtype: 'q8',
    cutoff: { floor: 0.065, ratio: 0.7 },
};

const MINILM: TextModel = {
    key: 'minilm-l12',
    id: 'Xenova/paraphrase-multilingual-MiniLM-L12-v2',
    dtype: 'q8',
    maxChars: 2_000,
    cutoff: { floor: 0.62, ratio: 0.9 },
};

export const AI_PRESETS: Record<AIPresetName, AIPreset> = {
    low: {
        image: { ...MSIGLIP, textDtype: 'q8' },
        text: MINILM,
        caption: { task: '<CAPTION>', visionDtype: 'q8' },
        ocr: 'tiny',
        keepAliveMs: 2 * 60_000,
        videoFrames: 8,
    },
    medium: {
        image: { ...MSIGLIP, textDtype: 'fp32' },
        text: MINILM,
        caption: { task: '<DETAILED_CAPTION>', visionDtype: 'q8' },
        ocr: 'small',
        keepAliveMs: 5 * 60_000,
        videoFrames: 16,
    },
    high: {
        image: { ...MSIGLIP, textDtype: 'fp32' },
        text: MINILM,
        caption: { task: '<DETAILED_CAPTION>', visionDtype: 'fp32' },
        ocr: 'small',
        keepAliveMs: 15 * 60_000,
        videoFrames: 32,
    },
};

export const getAIPreset = async () => AI_PRESETS[(await getSettings()).aiPreset];
