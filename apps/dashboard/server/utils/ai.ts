import { promises as fsp } from 'node:fs';
import { mkdtemp, rm } from 'node:fs/promises';
import { cpus, tmpdir } from 'node:os';

import consola from 'consola';
import dayjs from 'dayjs';
import fluentFfmpeg from 'fluent-ffmpeg';
import { dirname, extname, join } from 'pathe';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import {
    AutoModel,
    AutoTokenizer,
    CLIPImageProcessor,
    CLIPVisionModelWithProjection,
    env,
    Florence2ForConditionalGeneration,
    Florence2Processor,
    mean_pooling,
    pipeline,
    type PreTrainedModel,
    type PreTrainedTokenizer,
    RawImage,
} from '@huggingface/transformers';

type ODResult = {
    bboxes: number[][];
    labels: string[];
};

env.allowLocalModels = true;
env.backends.onnx.logLevel = 'error';

export const setInferenceThreads = (threads: number) => {
    if (!env.backends.onnx.wasm) return;

    env.backends.onnx.wasm.numThreads = Math.max(1, Math.min(cpus().length, Math.floor(threads)));
};

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const CLIP_VISION_MODEL_ID = 'Xenova/clip-vit-base-patch32';
const CLIP_TEXT_MODEL_ID = 'sentence-transformers/clip-ViT-B-32-multilingual-v1';
const FLORENCE_MODEL_ID = 'onnx-community/Florence-2-base';
const SEMANTIC_TEXT_MODEL_ID = 'Xenova/paraphrase-multilingual-MiniLM-L12-v2';
const NER_MODEL_ID = 'Xenova/bert-base-multilingual-cased-ner-hrl';

const CLIP_CENTERING = 0.8;
const CLIP_NEUTRAL_PROMPTS = [
    'a photo',
    'an image',
    'a picture',
    'something',
    'a thing',
    'object',
    'photo of a thing',
    'bir fotoğraf',
    'bir resim',
    'bir şey',
    'nesne',
    'görüntü',
];

type ClipVisionBundle = {
    imageProcessor: CLIPImageProcessor;
    visionModel: CLIPVisionModelWithProjection;
};

type ClipTextBundle = {
    tokenizer: PreTrainedTokenizer;
    textModel: PreTrainedModel;
    projection: Float32Array;
    center: number[];
};

const normalizeVector = (vector: number[]) => {
    const magnitude = Math.hypot(...vector) || 1;
    return vector.map((value) => value / magnitude);
};

const loadClipProjection = async () => {
    const path = join(env.cacheDir!, CLIP_TEXT_MODEL_ID, '2_Dense', 'model.safetensors');

    let buffer = await fsp.readFile(path).catch(() => null);

    if (!buffer) {
        const response = await fetch(
            `${env.remoteHost}${CLIP_TEXT_MODEL_ID}/resolve/main/2_Dense/model.safetensors`,
        );
        if (!response.ok)
            throw new Error(`Could not download the CLIP projection (${response.status})`);

        buffer = Buffer.from(await response.arrayBuffer());
        await fsp.mkdir(dirname(path), { recursive: true });
        await fsp.writeFile(`${path}.tmp`, buffer);
        await fsp.rename(`${path}.tmp`, path);
    }

    const headerLength = Number(buffer.readBigUInt64LE(0));
    const header = JSON.parse(buffer.subarray(8, 8 + headerLength).toString());
    const [start, end] = header['linear.weight'].data_offsets as [number, number];

    return new Float32Array(
        buffer.buffer.slice(
            buffer.byteOffset + 8 + headerLength + start,
            buffer.byteOffset + 8 + headerLength + end,
        ),
    );
};

type FlorenceBundle = {
    model: Florence2ForConditionalGeneration;
    processor: Florence2Processor;
};

class AIService {
    private static instance: AIService | null = null;

    public readonly CLIP_EMBEDDING_DIM = 512;
    public readonly SEMANTIC_TEXT_EMBEDDING_DIM = 384;

    public get IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS() {
        const list = ['.jpg', '.jpeg', '.png', '.webp', '.bmp'];
        return list.concat(list.map((ext) => ext.toUpperCase()));
    }

    public get VIDEO_EMBEDDING_SUPPORTED_EXTENSIONS() {
        const list = [
            '.mp4',
            '.webm',
            '.mov',
            '.avi',
            '.mkv',
            '.flv',
            '.wmv',
            '.m4v',
            '.mpg',
            '.mpeg',
            '.3gp',
        ];
        return list.concat(list.map((ext) => ext.toUpperCase()));
    }

    public get TEXT_EMBEDDING_SUPPORTED_EXTENSIONS() {
        const list = ['.txt', '.md', '.rtf', '.html', '.xml', '.json', '.csv'];
        return list.concat(list.map((ext) => ext.toUpperCase()));
    }

    private clipVisionPromise: Promise<ClipVisionBundle> | null = null;
    private clipTextPromise: Promise<ClipTextBundle> | null = null;
    private florencePromise: Promise<FlorenceBundle> | null = null;
    private semanticPipelinePromise: Promise<any> | null = null;
    private nerPipelinePromise: Promise<any> | null = null;

    public static getInstance() {
        if (!this.instance) {
            this.instance = new AIService();
        }
        return this.instance;
    }

    private async getClipVision() {
        if (!this.clipVisionPromise) {
            consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Initializing CLIP model...`);

            this.clipVisionPromise = Promise.all([
                CLIPImageProcessor.from_pretrained(CLIP_VISION_MODEL_ID),
                CLIPVisionModelWithProjection.from_pretrained(CLIP_VISION_MODEL_ID, {
                    dtype: 'q8',
                }),
            ]).then(([imageProcessor, visionModel]) => {
                consola.success(
                    `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - CLIP model initialized successfully.`,
                );
                return { imageProcessor, visionModel };
            });
        }

        return this.clipVisionPromise;
    }

    private async getClipText() {
        if (!this.clipTextPromise) {
            this.clipTextPromise = Promise.all([
                AutoTokenizer.from_pretrained(CLIP_TEXT_MODEL_ID),
                AutoModel.from_pretrained(CLIP_TEXT_MODEL_ID, {
                    dtype: 'fp32',
                    model_file_name:
                        process.arch === 'arm64' ? 'model_qint8_arm64' : 'model_quint8_avx2',
                }),
                loadClipProjection(),
            ]).then(async ([tokenizer, textModel, projection]) => {
                const bundle = { tokenizer, textModel, projection, center: [] as number[] };
                const prompts = await Promise.all(
                    CLIP_NEUTRAL_PROMPTS.map((prompt) => this.encodeClipText(bundle, prompt)),
                );

                bundle.center = normalizeVector(
                    prompts.reduce((sum, prompt) => sum.map((value, i) => value + prompt[i]!)),
                );

                return bundle;
            });

            this.clipTextPromise.catch(() => (this.clipTextPromise = null));
        }

        return this.clipTextPromise;
    }

    private async encodeClipText(
        { tokenizer, textModel, projection }: ClipTextBundle,
        text: string,
    ) {
        const inputs = tokenizer(text, { padding: true, truncation: true });
        const { last_hidden_state } = await textModel(inputs);
        const pooled = mean_pooling(last_hidden_state, inputs.attention_mask).data as Float32Array;

        return normalizeVector(
            Array.from({ length: this.CLIP_EMBEDDING_DIM }, (_, row) => {
                let sum = 0;

                for (let column = 0; column < pooled.length; column++)
                    sum += projection[row * pooled.length + column]! * pooled[column]!;

                return sum;
            }),
        );
    }

    private async getFlorence() {
        if (!this.florencePromise) {
            consola.info(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Initializing Florence-2 model...`,
            );

            this.florencePromise = Promise.all([
                Florence2ForConditionalGeneration.from_pretrained(FLORENCE_MODEL_ID, {
                    dtype: 'q8',
                }),
                Florence2Processor.from_pretrained(FLORENCE_MODEL_ID),
            ] as [Promise<Florence2ForConditionalGeneration>, Promise<Florence2Processor>]).then(
                ([model, processor]) => {
                    consola.success(
                        `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Florence-2 model initialized successfully.`,
                    );
                    return { model, processor };
                },
            );
        }

        return this.florencePromise;
    }

    private async getSemanticTextPipeline() {
        if (!this.semanticPipelinePromise) {
            this.semanticPipelinePromise = pipeline('feature-extraction', SEMANTIC_TEXT_MODEL_ID, {
                dtype: 'q8',
            });
        }

        return this.semanticPipelinePromise;
    }

    private async getNerPipeline() {
        if (!this.nerPipelinePromise) {
            this.nerPipelinePromise = pipeline('token-classification', NER_MODEL_ID, {
                dtype: 'q8',
            });
        }
        return this.nerPipelinePromise;
    }

    public async createImageEmbedding(imagePath: string): Promise<number[]> {
        const { imageProcessor, visionModel } = await this.getClipVision();
        const image = await RawImage.fromURL(imagePath);
        const imageInputs = await imageProcessor(image);
        const { image_embeds } = await visionModel(imageInputs);
        const normalizedEmbeds = image_embeds.normalize(2, -1);

        return Array.from(normalizedEmbeds.data);
    }

    public async createClipQueryEmbedding(text: string) {
        const bundle = await this.getClipText();
        const embedding = await this.encodeClipText(bundle, text);

        return embedding.map((value, i) => value - CLIP_CENTERING * bundle.center[i]!);
    }

    public async runFlorenceTask(imagePath: string, task: string) {
        const { model, processor } = await this.getFlorence();
        const image = await RawImage.fromURL(imagePath);

        const prompts = processor.construct_prompts(task);
        const inputs = await processor(image, prompts);

        const generatedIds = await model.generate({
            ...inputs,
            max_new_tokens: 1024,
            use_cache: true,
        });

        const generatedText = processor.batch_decode(generatedIds as never, {
            skip_special_tokens: false,
        })[0]!;

        const result = processor.post_process_generation(generatedText, task, image.size);
        return result[task] ?? null;
    }

    public async performObjectDetection(imagePath: string) {
        const result = (await this.runFlorenceTask(imagePath, '<OD>')) as ODResult;
        return result?.bboxes && result?.labels ? result : null;
    }

    public async performOcrOnImage(imagePath: string) {
        const result = (await this.runFlorenceTask(imagePath, '<OCR_WITH_REGION>')) as {
            labels?: string[];
        } | null;

        return (result?.labels ?? []).join(' ').replace(/\s+/g, ' ').trim();
    }

    public async createSemanticTextEmbedding(text: string) {
        const input = text.replace(/\s+/g, ' ').trim();
        if (!input.length) return null;

        const extractor = await this.getSemanticTextPipeline();
        const result = await extractor(input.slice(0, 4000), {
            pooling: 'mean',
            normalize: true,
        });

        if (Array.isArray(result) && typeof result[0] === 'number') {
            return result as number[];
        }

        const data = result?.data;
        if (data && typeof data.length === 'number') {
            return Array.from(data) as number[];
        }

        return null;
    }

    public async generateImageCaption(imagePath: string) {
        const result = await this.runFlorenceTask(imagePath, '<CAPTION>');
        return typeof result === 'string' ? result.replace(/\s+/g, ' ').trim() : '';
    }

    public async detectPII(filePath: string, ocrText?: string | null) {
        const isImage = ai.IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(filePath));
        const isText = ai.TEXT_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extname(filePath));

        const text = isImage
            ? (ocrText ?? (await this.performOcrOnImage(filePath)))
            : isText
              ? await readUtf8Snippet(filePath, 120_000)
              : '';

        const reasons = findPii(text);

        if (isImage) findDocumentPii(text).forEach((reason) => reasons.add(reason));

        if (isImage && hasDocumentEvidence(text)) {
            const detection = await this.performObjectDetection(filePath).catch(() => null);

            for (const label of detection?.labels ?? []) {
                const reason = getVisualPiiReason(label);
                if (reason) reasons.add(reason);
            }
        }

        if (reasons.size) {
            const ner = await this.getNerPipeline();
            const entities = await ner(text.slice(0, 2000), { aggregation_strategy: 'simple' });

            if (
                (entities as any[]).some(
                    (entity) =>
                        (entity.entity_group ?? entity.entity)?.endsWith('PER') &&
                        entity.score >= 0.85 &&
                        entity.word.replace(/#/g, '').trim().length > 2,
                )
            )
                reasons.add('PERSON_NAME');
        }

        return { piiDetected: reasons.size > 0, reasons: [...reasons] };
    }

    public async createVideoEmbedding(options: { filePath: string }): Promise<number[]> {
        const workDir = await mkdtemp(join(tmpdir(), 'fileship-video-'));
        const settings = await getSettings();
        const configuredMaxFrames = settings.aiVideoMaxFrames;
        const maxFrames = Math.max(
            1,
            Math.min(
                120,
                Number.isFinite(configuredMaxFrames) && configuredMaxFrames > 0
                    ? Math.floor(configuredMaxFrames)
                    : 24,
            ),
        );
        const configuredSampleInterval = settings.aiVideoSampleIntervalSeconds;
        const sampleIntervalSeconds = Math.max(
            1,
            Math.min(
                3_600,
                Number.isFinite(configuredSampleInterval) && configuredSampleInterval > 0
                    ? configuredSampleInterval
                    : 10,
            ),
        );

        try {
            await new Promise<void>((resolve, reject) => {
                fluentFfmpeg(options.filePath)
                    .inputOptions('-threads 1')
                    .videoFilters([`fps=1/${sampleIntervalSeconds}`, 'scale=336:-2'])
                    .frames(maxFrames)
                    .outputOptions('-threads 1')
                    .output(join(workDir, 'frame-%03d.jpg'))
                    .on('end', () => resolve())
                    .on('error', (err) => reject(err))
                    .run();
            });

            const files = (await fsp.readdir(workDir))
                .filter((f) => f.startsWith('frame-') && f.endsWith('.jpg'))
                .sort();

            const embeddings: number[][] = [];
            for (const f of files) {
                const framePath = join(workDir, f);
                try {
                    embeddings.push(await this.createImageEmbedding(framePath));
                } finally {
                    await fsp.unlink(framePath).catch(() => null);
                }
            }

            if (!embeddings.length) throw new Error('No frames could be read from the video');

            return normalizeVector(
                embeddings[0]!.map(
                    (_, i) =>
                        embeddings.reduce((sum, embedding) => sum + embedding[i]!, 0) /
                        embeddings.length,
                ),
            );
        } finally {
            await rm(workDir, { recursive: true, force: true });
        }
    }
}

export const ai = AIService.getInstance();
