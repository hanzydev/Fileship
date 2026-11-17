import type { H3Event } from 'h3';

const MAX_IMAGE_SIZE = 20 * 1024 * 1024;

export const readAIImage = async (event: H3Event, scope: string, limit: number) => {
    userOnly(event);

    const currentUser = event.context.user!;

    if (!(await isAiEnabled()) || (currentUser.aiSettings as any)?.enabled === false) {
        throw apiError(403, 'errorsSystem.aiDisabled');
    }

    if (!aiService.up) throw apiError(503, 'errorsSystem.aiServiceUnavailable');

    if (+(getRequestHeader(event, 'content-length') ?? 0) > MAX_IMAGE_SIZE) {
        throw apiError(413, 'errorsSystem.imageTooLarge');
    }

    await rateLimit(`${scope}:${currentUser.id}`, limit, 60_000);

    const image = (await readFormData(event)).get('image');

    if (!image || typeof image === 'string' || !image.type.startsWith('image/')) {
        throw apiError(400, 'errorsSystem.imageMissing');
    }

    if (image.size > MAX_IMAGE_SIZE) {
        throw apiError(413, 'errorsSystem.imageTooLarge');
    }

    return Buffer.from(await image.arrayBuffer());
};
