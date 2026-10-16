import fluentFfmpeg from 'fluent-ffmpeg';
import sharp, { type Sharp } from 'sharp';

import { getConfig, getSessionOptions, unloadModels, useAIModel } from './models';
import { recognizeImageText } from './ocr';
import type { AIMethods, ImageModelConfig, PrivacyEntity, TextModelConfig } from './types';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import {
    AutoProcessor,
    AutoTokenizer,
    Florence2ForConditionalGeneration,
    Florence2Processor,
    pipeline,
    RawImage,
    SiglipTextModel,
    SiglipVisionModel,
} from '@huggingface/transformers';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const FLORENCE_MODEL_ID = 'onnx-community/Florence-2-base';
const PRIVACY_FILTER_MODEL_ID = 'openai/privacy-filter';

const PRIVACY_FILTER_MAX_CHARS = 20_000;
const QUERY_MODEL_KEEP_ALIVE_MS = 30 * 60_000;
const IMAGE_SIZE = 1024;
const OCR_IMAGE_SIZE = 1600;
const VIDEO_FRAME_TIMEOUT_MS = 60_000;
const IMAGE_CACHE_MS = 60_000;

const normalize = (vector: ArrayLike<number>) => {
    let magnitude = 0;
    for (let i = 0; i < vector.length; i++) magnitude += vector[i]! * vector[i]!;
    magnitude = Math.sqrt(magnitude) || 1;

    return Array.from(vector, (value) => value / magnitude);
};

const useImageVision = <R>(model: ImageModelConfig, fn: (vision: any) => Promise<R>) =>
    useAIModel(
        `image:${model.key}`,
        async () => ({
            processor: await AutoProcessor.from_pretrained(model.id),
            model: await SiglipVisionModel.from_pretrained(model.id, {
                dtype: model.visionDtype as any,
                session_options: getSessionOptions(),
            }),
        }),
        (vision) => vision.model.dispose(),
        fn,
    );

const useImageText = <R>(model: ImageModelConfig, fn: (text: any) => Promise<R>) =>
    useAIModel(
        `image-query:${model.key}:${model.textDtype}`,
        async () => ({
            tokenizer: await AutoTokenizer.from_pretrained(model.id),
            model: await SiglipTextModel.from_pretrained(model.id, {
                dtype: model.textDtype as any,
                session_options: getSessionOptions(),
            }),
        }),
        (text) => text.model.dispose(),
        fn,
        QUERY_MODEL_KEEP_ALIVE_MS,
    );

const useTextModel = <R>(
    model: TextModelConfig,
    fn: (extractor: any) => Promise<R>,
    query = false,
) =>
    useAIModel(
        `text:${model.key}`,
        () =>
            pipeline('feature-extraction', model.id, {
                dtype: model.dtype as any,
                session_options: getSessionOptions(),
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
                session_options: getSessionOptions(),
            }),
            processor: await Florence2Processor.from_pretrained(FLORENCE_MODEL_ID),
        }),
        (florence) => florence.model.dispose(),
        fn,
    );

const usePrivacyFilter = <R>(fn: (filter: any) => Promise<R>) =>
    useAIModel(
        'privacy-filter',
        () =>
            pipeline('token-classification', PRIVACY_FILTER_MODEL_ID, {
                dtype: 'q4',
                session_options: getSessionOptions(),
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

let cachedImage: {
    path: string;
    image: Promise<RawImage>;
    timer: ReturnType<typeof setTimeout>;
} | null = null;

const loadImage = (path: string) => {
    if (cachedImage?.path !== path) {
        clearTimeout(cachedImage?.timer);
        cachedImage = { path, image: toRawImage(sharp(path), IMAGE_SIZE), timer: undefined! };
    }

    const entry = cachedImage;

    clearTimeout(entry.timer);
    entry.timer = setTimeout(() => cachedImage === entry && (cachedImage = null), IMAGE_CACHE_MS);
    entry.image.catch(() => cachedImage === entry && (cachedImage = null));

    return entry.image;
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

const embedRawImage = (image: RawImage) =>
    useImageVision(getConfig().preset.image, async ({ processor, model }) =>
        normalize((await model(await processor(image))).pooler_output.data),
    );

export const methods: AIMethods = {
    async warmUp() {
        const { preset } = getConfig();

        await useImageText(preset.image, async () => null);
        await useTextModel(preset.text, async () => null, true);
    },

    async unload() {
        await unloadModels();
    },

    async embedImage(path) {
        return embedRawImage(await loadImage(path));
    },

    async embedImageQuery(query) {
        return useImageText(getConfig().preset.image, async (text) => {
            const inputs = text.tokenizer(query.toLowerCase(), {
                padding: 'max_length',
                truncation: true,
                max_length: 64,
            });

            return normalize((await text.model(inputs)).pooler_output.data);
        });
    },

    async embedVideo(path, duration) {
        const { videoFrames } = getConfig().preset;
        const times = duration
            ? Array.from({ length: videoFrames }, (_, i) => (duration * (i + 0.5)) / videoFrames)
            : Array.from({ length: videoFrames }, (_, i) => i * 10);

        const embeddings: number[][] = [];

        for (const time of times) {
            const frame = await readVideoFrame(path, time).catch(() => null);
            if (!frame?.length) continue;

            embeddings.push(await embedRawImage(await toRawImage(sharp(frame), IMAGE_SIZE)));
        }

        if (!embeddings.length) throw new Error('No frames could be read from the video');

        return normalize(
            embeddings[0]!.map(
                (_, i) =>
                    embeddings.reduce((sum, embedding) => sum + embedding[i]!, 0) /
                    embeddings.length,
            ),
        );
    },

    async embedText(text, query = false) {
        const model = getConfig().preset.text;
        const input = text.replace(/\s+/g, ' ').trim().slice(0, model.maxChars);

        if (!input) return null;

        return useTextModel(
            model,
            async (extractor) =>
                normalize((await extractor(input, { pooling: 'mean', normalize: true })).data),
            query,
        );
    },

    async captionImage(path) {
        const { caption } = getConfig().preset;
        const image = await loadImage(path);

        return useFlorence(caption.visionDtype, async ({ model, processor }) => {
            const inputs = await processor(image, processor.construct_prompts(caption.task));
            const generated = await model.generate({ ...inputs, max_new_tokens: 256 });
            const text = processor.batch_decode(generated, { skip_special_tokens: false })[0];
            const result = processor.post_process_generation(text, caption.task, image.size)[
                caption.task
            ];

            return typeof result === 'string' ? result.replace(/\s+/g, ' ').trim() : '';
        });
    },

    async recognizeText(image) {
        const { data, info } = await sharp(image)
            .rotate()
            .resize(OCR_IMAGE_SIZE, OCR_IMAGE_SIZE, { fit: 'inside', withoutEnlargement: true })
            .png()
            .toBuffer({ resolveWithObject: true });

        return recognizeImageText(
            data.buffer.slice(data.byteOffset, data.byteOffset + data.byteLength) as ArrayBuffer,
            info,
            getConfig().preset.ocr,
        );
    },

    async findPrivacyEntities(text) {
        const input = text.slice(0, PRIVACY_FILTER_MAX_CHARS);
        if (!input.trim()) return [];

        const entities: PrivacyEntity[] = await usePrivacyFilter((filter) =>
            filter(input, { aggregation_strategy: 'simple' }),
        );

        return entities.map(({ entity_group, score, word }) => ({ entity_group, score, word }));
    },
};
