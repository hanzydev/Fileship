import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

import { createAIClient } from '@fileship/ai';

import { dataDirectory } from './directory';

const PRIVACY_FILTER_MAX_CHARS = 20_000;

const client = createAIClient({
    config: async () => ({
        preset: await getAIPreset(),
        threads: (await getSettings()).aiInferenceThreads,
        cacheDir: join(dataDirectory, 'models'),
    }),
    log: (message) => consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${message}`),
});

export const unloadAIModels = () => client.unload();

export const stopAIWorker = () => client.terminate();

export const ai = {
    warmUp: client.warmUp,
    embedImage: client.embedImage,
    embedImageQuery: client.embedImageQuery,
    embedVideo: client.embedVideo,
    embedText: client.embedText,
    captionImage: client.captionImage,
    recognizeText: client.recognizeText,

    async detectPII(text: string, caption = '') {
        const input = text.slice(0, PRIVACY_FILTER_MAX_CHARS);
        if (!input.trim()) return { piiDetected: false, reasons: [] };

        const reasons = findPii(input, await client.findPrivacyEntities(input), caption);

        return { piiDetected: reasons.size > 0, reasons: [...reasons] };
    },
};
