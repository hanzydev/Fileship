const MAX_IMAGE_SIZE = 20 * 1024 * 1024;

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    if (!(await isAiEnabled()) || (currentUser.aiSettings as any)?.enabled === false) {
        throw apiError(403, 'errorsSystem.aiDisabled');
    }

    if (+(getRequestHeader(event, 'content-length') ?? 0) > MAX_IMAGE_SIZE) {
        throw apiError(413, 'errorsSystem.imageTooLarge');
    }

    await rateLimit(`ocr:${currentUser.id}`, 30, 60_000);

    const image = (await readFormData(event)).get('image');

    if (!image || typeof image === 'string' || !image.type.startsWith('image/')) {
        throw apiError(400, 'errorsSystem.imageMissing');
    }

    if (image.size > MAX_IMAGE_SIZE) {
        throw apiError(413, 'errorsSystem.imageTooLarge');
    }

    const text = await ai.recognizeText(new Uint8Array(await image.arrayBuffer())).catch(() => {
        throw apiError(422, 'errorsSystem.imageUnreadable');
    });

    return { text };
});
