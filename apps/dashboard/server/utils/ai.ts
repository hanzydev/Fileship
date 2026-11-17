import { FetchError, ofetch } from 'ofetch';

const PRIVACY_FILTER_MAX_CHARS = 20_000;
const REQUEST_TIMEOUT_MS = 10 * 60_000;

type Embedding = { model: string; vector: number[] };

export type PrivacyEntity = { group: string; score: number; word: string };

const serviceUrl = useRuntimeConfig().ai.serviceUrl;

export const aiService = {
    url: serviceUrl || 'http://127.0.0.1:3003',
    managed: !serviceUrl,
    up: false,
    modelsBytes: 0,
};

export const isAIServiceUnreachable = (error: unknown) =>
    error instanceof FetchError && (error.cause as any)?.cause?.code === 'ECONNREFUSED';

const aiFetch = ofetch.create({
    baseURL: aiService.url,
    method: 'POST',
    timeout: REQUEST_TIMEOUT_MS,
});

const request = async <T>(path: string, body?: FormData | Record<string, unknown>): Promise<T> => {
    const { aiPreset, aiInferenceThreads } = await getSettings();

    return aiFetch<T>(path, {
        body,
        headers: {
            'Fileship-AI-Preset': aiPreset,
            'Fileship-AI-Threads': String(aiInferenceThreads),
        },
    });
};

const images = (files: Buffer[]) => {
    const form = new FormData();
    for (const file of files) {
        form.append('image', new Blob([file as BlobPart], { type: 'image/png' }), 'image.png');
    }
    return form;
};

export const ai = {
    warmUp: () => request('/warmup'),

    embedImage: (image: Buffer) => request<Embedding>('/image/embed', images([image])),

    embedVideo: (frames: Buffer[]) => request<Embedding>('/image/embed', images(frames)),

    embedImageQuery: async (query: string) =>
        (await request<Embedding>('/text/image-query', { text: query })).vector,

    embedText: (text: string, query = false) =>
        request<{ model: string; vector: number[] | null }>('/text/embed', { text, query }),

    captionImage: async (image: Buffer) =>
        (await request<{ caption: string }>('/image/caption', images([image]))).caption,

    recognizeText: async (image: Buffer) =>
        (await request<{ text: string }>('/image/ocr', images([image]))).text,

    async detectPII(text: string, caption = '') {
        const input = text.slice(0, PRIVACY_FILTER_MAX_CHARS);
        if (!input.trim()) return { piiDetected: false, reasons: [] };

        const { entities } = await request<{ entities: PrivacyEntity[] }>('/text/entities', {
            text: input,
        });
        const reasons = findPii(input, entities, caption);

        return { piiDetected: reasons.size > 0, reasons: [...reasons] };
    },
};
