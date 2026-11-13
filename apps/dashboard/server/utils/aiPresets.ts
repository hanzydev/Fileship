export type AIPresetName = 'low' | 'medium' | 'high';

export type SearchCutoff = { floor: number; ratio: number };

type Model = { key: string; cutoff: SearchCutoff };

export type AIPreset = { image: Model; text: Model; videoFrames: number };

const MSIGLIP: Model = { key: 'msiglip-b16', cutoff: { floor: 0.065, ratio: 0.7 } };

const MINILM: Model = { key: 'minilm-l12', cutoff: { floor: 0.62, ratio: 0.9 } };

export const AI_PRESETS: Record<AIPresetName, AIPreset> = {
    low: { image: MSIGLIP, text: MINILM, videoFrames: 8 },
    medium: { image: MSIGLIP, text: MINILM, videoFrames: 16 },
    high: { image: MSIGLIP, text: MINILM, videoFrames: 32 },
};

export const getAIPreset = async () => AI_PRESETS[(await getSettings()).aiPreset];
