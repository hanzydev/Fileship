import { promises as fsp } from 'node:fs';
import { cpus } from 'node:os';

import consola from 'consola';
import dayjs from 'dayjs';
import fluentFfmpeg from 'fluent-ffmpeg';
import { dirname, join } from 'pathe';
import sharp, { type Sharp } from 'sharp';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import {
    AutoProcessor,
    AutoTokenizer,
    env,
    Florence2ForConditionalGeneration,
    Florence2Processor,
    pipeline,
    RawImage,
    SiglipTextModel,
    SiglipVisionModel,
} from '@huggingface/transformers';

import type { ImageModel, TextModel } from './aiPresets';
import { dataDirectory } from './directory';
import type { PrivacyEntity } from './pii';

env.allowLocalModels = true;
env.cacheDir = join(dataDirectory, 'models');
env.backends.onnx.logLevel = 'error';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const FLORENCE_MODEL_ID = 'onnx-community/Florence-2-base';
const PRIVACY_FILTER_MODEL_ID = 'openai/privacy-filter';

const PRIVACY_FILTER_MAX_CHARS = 20_000;
const QUERY_MODEL_KEEP_ALIVE_MS = 30 * 60_000;
const IMAGE_SIZE = 1024;
const OCR_IMAGE_SIZE = 1600;
const VIDEO_FRAME_TIMEOUT_MS = 60_000;

const log = (message: string) =>
    consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${message}`);

export const getSessionOptions = async () => {
    const { aiInferenceThreads } = await getSettings();

    return {
        intraOpNumThreads: Math.max(1, Math.min(cpus().length, aiInferenceThreads)),
        interOpNumThreads: 1,
        enableCpuMemArena: false,
        enableMemPattern: false,
    };
};

export const downloadModelFile = async (url: string, name: string) => {
    const path = join(env.cacheDir!, name);
    if (await fsp.stat(path).catch(() => null)) return path;

    log(`Downloading ${name}...`);

    const response = await fetch(url);
    if (!response.ok) throw new Error(`Could not download ${name} (${response.status})`);

    await fsp.mkdir(dirname(path), { recursive: true });
    await fsp.writeFile(`${path}.tmp`, Buffer.from(await response.arrayBuffer()));
    await fsp.rename(`${path}.tmp`, path);

    return path;
};

type LoadedModel = {
    value: Promise<any>;
    users: number;
    keepAliveMs?: number;
    timer?: NodeJS.Timeout;
    dispose: (value: any) => Promise<unknown>;
};

const loadedModels = new Map<string, LoadedModel>();

const unloadModel = async (key: string, entry: LoadedModel) => {
    if (loadedModels.get(key) !== entry || entry.users) return;

    loadedModels.delete(key);
    await entry.dispose(await entry.value).catch(() => null);
};

export const useAIModel = async <T, R>(
    key: string,
    load: () => Promise<T>,
    dispose: (value: T) => Promise<unknown>,
    fn: (value: T) => Promise<R>,
    keepAliveMs?: number,
): Promise<R> => {
    let entry = loadedModels.get(key);

    if (!entry) {
        log(`Loading ${key}...`);

        entry = { value: load(), users: 0, keepAliveMs, dispose };
        loadedModels.set(key, entry);

        const loading = entry;
        entry.value.catch(() => loadedModels.get(key) === loading && loadedModels.delete(key));
    }

    clearTimeout(entry.timer);
    if (keepAliveMs !== undefined) entry.keepAliveMs = keepAliveMs;
    entry.users++;

    try {
        return await fn(await entry.value);
    } finally {
        const current = entry;

        if (!--current.users) {
            const idle = current.keepAliveMs ?? (await getAIPreset()).keepAliveMs;

            if (idle) current.timer = setTimeout(() => unloadModel(key, current), idle);
            else await unloadModel(key, current);
        }
    }
};

export const unloadAIModels = async () => {
    await Promise.all(
        [...loadedModels.entries()].map(([key, entry]) => {
            clearTimeout(entry.timer);
            return entry.users ? null : unloadModel(key, entry);
        }),
    );
};

const normalize = (vector: ArrayLike<number>) => {
    let magnitude = 0;
    for (let i = 0; i < vector.length; i++) magnitude += vector[i]! * vector[i]!;
    magnitude = Math.sqrt(magnitude) || 1;

    return Array.from(vector, (value) => value / magnitude);
};

const useImageVision = <R>(model: ImageModel, fn: (vision: any) => Promise<R>) =>
    useAIModel(
        `image:${model.key}`,
        async () => {
            return {
                processor: await AutoProcessor.from_pretrained(model.id),
                model: await SiglipVisionModel.from_pretrained(model.id, {
                    dtype: model.visionDtype as any,
                    session_options: await getSessionOptions(),
                }),
            };
        },
        (vision) => vision.model.dispose(),
        fn,
    );

const useImageText = <R>(model: ImageModel, fn: (text: any) => Promise<R>) =>
    useAIModel(
        `image-query:${model.key}:${model.textDtype}`,
        async () => ({
            tokenizer: await AutoTokenizer.from_pretrained(model.id),
            model: await SiglipTextModel.from_pretrained(model.id, {
                dtype: model.textDtype as any,
                session_options: await getSessionOptions(),
            }),
        }),
        (text) => text.model.dispose(),
        fn,
        QUERY_MODEL_KEEP_ALIVE_MS,
    );

const useTextModel = <R>(model: TextModel, fn: (extractor: any) => Promise<R>, query = false) =>
    useAIModel(
        `text:${model.key}`,
        async () =>
            pipeline('feature-extraction', model.id, {
                dtype: model.dtype as any,
                session_options: await getSessionOptions(),
            }),
        (extractor) => extractor.dispose(),
        fn,
        query ? QUERY_MODEL_KEEP_ALIVE_MS : undefined,
    );

const useFlorence = <R>(visionDtype: string, fn: (florence: any) => Promise<R>) =>
    useAIModel(
        `caption:${visionDtype}`,
        async () => ({
            model: await Florence2ForConditionalGeneration.from_pretrained(FLORENCE_MODEL_ID, {
                dtype: {
                    vision_encoder: visionDtype,
                    encoder_model: 'q8',
                    decoder_model_merged: 'q8',
                    embed_tokens: 'q8',
                } as any,
                session_options: await getSessionOptions(),
            }),
            processor: await Florence2Processor.from_pretrained(FLORENCE_MODEL_ID),
        }),
        (florence) => florence.model.dispose(),
        fn,
    );

const usePrivacyFilter = <R>(fn: (filter: any) => Promise<R>) =>
    useAIModel(
        'privacy-filter',
        async () =>
            pipeline('token-classification', PRIVACY_FILTER_MODEL_ID, {
                dtype: 'q4',
                session_options: await getSessionOptions(),
            }),
        (filter) => filter.dispose(),
        fn,
    );

const toRawImage = async (input: Sharp, size: number) => {
    const { data, info } = await input
        .rotate()
        .resize(size, size, { fit: 'inside', withoutEnlargement: true })
        .removeAlpha()
        .raw()
        .toBuffer({ resolveWithObject: true });

    return new RawImage(new Uint8ClampedArray(data), info.width, info.height, 3);
};

const readVideoFrame = (path: string, seconds: number) =>
    new Promise<Buffer>((resolve, reject) => {
        const chunks: Buffer[] = [];
        const command = fluentFfmpeg(path)
            .seekInput(seconds)
            .inputOptions('-threads 1')
            .frames(1)
            .videoFilters('scale=448:-2')
            .outputOptions(['-f image2pipe', '-vcodec mjpeg', '-threads 1'])
            .on('error', reject);

        const timer = setTimeout(() => {
            command.kill('SIGKILL');
            reject(new Error('Reading a video frame timed out'));
        }, VIDEO_FRAME_TIMEOUT_MS);

        const stream = command.pipe();
        stream.on('data', (chunk: Buffer) => chunks.push(chunk));
        stream.on('end', () => {
            clearTimeout(timer);
            resolve(Buffer.concat(chunks));
        });
    });

class AIService {
    public async warmUp() {
        const preset = await getAIPreset();

        await useImageText(preset.image, async () => null);
        await useTextModel(preset.text, async () => null, true);
    }

    public async loadImage(path: string) {
        return toRawImage(sharp(path), IMAGE_SIZE);
    }

    public async embedImage(image: RawImage) {
        const { image: model } = await getAIPreset();

        return useImageVision(model, async ({ processor, model: vision }) => {
            return normalize((await vision(await processor(image))).pooler_output.data);
        });
    }

    public async embedImageQuery(query: string) {
        const { image: model } = await getAIPreset();

        return useImageText(model, async (text) => {
            const inputs = text.tokenizer(query.toLowerCase(), {
                padding: 'max_length',
                truncation: true,
                max_length: 64,
            });

            return normalize((await text.model(inputs)).pooler_output.data);
        });
    }

    public async embedVideo(path: string, duration?: number) {
        const { videoFrames } = await getAIPreset();
        const times = duration
            ? Array.from({ length: videoFrames }, (_, i) => (duration * (i + 0.5)) / videoFrames)
            : Array.from({ length: videoFrames }, (_, i) => i * 10);

        const embeddings: number[][] = [];

        for (const time of times) {
            const frame = await readVideoFrame(path, time).catch(() => null);
            if (!frame?.length) continue;

            embeddings.push(await this.embedImage(await toRawImage(sharp(frame), IMAGE_SIZE)));
        }

        if (!embeddings.length) throw new Error('No frames could be read from the video');

        return normalize(
            embeddings[0]!.map(
                (_, i) =>
                    embeddings.reduce((sum, embedding) => sum + embedding[i]!, 0) /
                    embeddings.length,
            ),
        );
    }

    public async embedText(text: string, query = false) {
        const { text: model } = await getAIPreset();
        const input = text.replace(/\s+/g, ' ').trim().slice(0, model.maxChars);

        if (!input) return null;

        return useTextModel(
            model,
            async (extractor) =>
                normalize((await extractor(input, { pooling: 'mean', normalize: true })).data),
            query,
        );
    }

    public async captionImage(image: RawImage) {
        const { caption } = await getAIPreset();

        return useFlorence(caption.visionDtype, async ({ model, processor }) => {
            const inputs = await processor(image, processor.construct_prompts(caption.task));
            const generated = await model.generate({ ...inputs, max_new_tokens: 256 });
            const text = processor.batch_decode(generated, { skip_special_tokens: false })[0];
            const result = processor.post_process_generation(text, caption.task, image.size)[
                caption.task
            ];

            return typeof result === 'string' ? result.replace(/\s+/g, ' ').trim() : '';
        });
    }

    public async recognizeText(path: string) {
        const { ocr } = await getAIPreset();
        const { data, info } = await sharp(path)
            .rotate()
            .resize(OCR_IMAGE_SIZE, OCR_IMAGE_SIZE, { fit: 'inside', withoutEnlargement: true })
            .png()
            .toBuffer({ resolveWithObject: true });

        return recognizeImageText(
            data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer,
            info,
            ocr,
        );
    }

    public async detectPII(text: string, caption = '') {
        const input = text.slice(0, PRIVACY_FILTER_MAX_CHARS);
        if (!input.trim()) return { piiDetected: false, reasons: [] };

        const entities: PrivacyEntity[] = await usePrivacyFilter((filter) =>
            filter(input, { aggregation_strategy: 'simple' }),
        );
        const reasons = findPii(input, entities, caption);

        return { piiDetected: reasons.size > 0, reasons: [...reasons] };
    }
}

export const ai = new AIService();
