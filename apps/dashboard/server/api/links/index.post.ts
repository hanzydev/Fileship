import { z } from 'zod';

const validationSchema = z.object({
    url: linkUrlSchema,
    code: linkCodeSchema.nullish(),
    password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
    maxClicks: z.number().int().min(0, 'Max clicks must be at least 0').nullish(),
    expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
});

export default defineEventHandler(async (event) => {
    linkShortenerOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.code) await assertLinkCodeFree(body.data.code);

    const link = await prisma.link.create({
        data: {
            url: body.data.url,
            code: body.data.code || (await createLinkCode()),
            password: body.data.password || null,
            maxClicks: body.data.maxClicks || 0,
            expiresAt: body.data.expiration ? new Date(Date.now() + body.data.expiration) : null,
            authorId: currentUser.id,
        },
    });

    const serialized = serializeLink(event, link, currentUser.domains);

    await createLog(event, {
        action: 'Shorten Link',
        message: `Shortened ${link.url} to ${link.code}`,
    });

    sendToUser(currentUser.id, 'link:create', serialized);

    return serialized;
});
