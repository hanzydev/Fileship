import consola from 'consola';
import dayjs from 'dayjs';

import {
    AutoProcessor,
    AutoTokenizer,
    CLIPTextModelWithProjection,
    CLIPVisionModelWithProjection,
    RawImage,
} from '@xenova/transformers';

const MODEL_ID = 'Xenova/clip-vit-large-patch14';

export const IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS = ['.jpg', '.jpeg', '.png', '.webp'];

class CLIPSingleton {
    private static instance: CLIPSingleton | null = null;

    public tokenizer!: AutoTokenizer;
    public processor!: AutoProcessor;
    public text_model!: CLIPTextModelWithProjection;
    public vision_model!: CLIPVisionModelWithProjection;

    public static async getInstance(): Promise<CLIPSingleton> {
        if (!this.instance) {
            this.instance = new CLIPSingleton();
            await this.instance.init();
        }
        return this.instance;
    }

    private async init() {
        consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Initializing CLIP model...`);

        [this.tokenizer, this.processor, this.text_model, this.vision_model] = await Promise.all([
            AutoTokenizer.from_pretrained(MODEL_ID),
            AutoProcessor.from_pretrained(MODEL_ID),
            CLIPTextModelWithProjection.from_pretrained(MODEL_ID),
            CLIPVisionModelWithProjection.from_pretrained(MODEL_ID),
        ]);

        consola.success(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - CLIP model initialized successfully.`,
        );
    }

    public async createImageEmbedding(imagePath: string): Promise<number[]> {
        const image = await RawImage.fromURL(imagePath);
        // @ts-expect-error fix processor type
        const image_inputs = await this.processor(image);
        const { image_embeds } = await this.vision_model(image_inputs);
        const normalized_embeds = image_embeds.normalize(2, -1);
        return Array.from(normalized_embeds.data);
    }

    public async createTextEmbedding(text: string): Promise<number[]> {
        // @ts-expect-error fix tokenizer type
        const text_inputs = this.tokenizer(text, {
            padding: true,
            truncation: true,
            return_tensors: 'pt',
        });
        const { text_embeds } = await this.text_model(text_inputs);
        const normalized_embeds = text_embeds.normalize(2, -1);
        return Array.from(normalized_embeds.data);
    }
}

export const getClipInstance = () => CLIPSingleton.getInstance();
