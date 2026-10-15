const MAX_IMAGE_SIZE = 20 * 1024 * 1024;

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    if (!(await isAiEnabled()) || (currentUser.aiSettings as any)?.enabled === false) {
        throw createError({
            status: 403,
            message: 'AI features are turned off',
        });
    }

    if (+(getRequestHeader(event, 'content-length') ?? 0) > MAX_IMAGE_SIZE) {
        throw createError({
            status: 413,
            message: 'The image is too large',
        });
    }

    await rateLimit(`ocr:${currentUser.id}`, 30, 60_000);

    const image = (await readFormData(event)).get('image');

    if (!image || typeof image === 'string' || !image.type.startsWith('image/')) {
        throw createError({
            status: 400,
            message: 'Missing image',
        });
    }

    if (image.size > MAX_IMAGE_SIZE) {
        throw createError({
            status: 413,
            message: 'The image is too large',
        });
    }

    const text = await ai.recognizeText(new Uint8Array(await image.arrayBuffer())).catch(() => {
        throw createError({
            status: 422,
            message: 'Could not read the image',
        });
    });

    return { text };
});
