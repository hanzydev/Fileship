import { z } from 'zod';

const validationSchema = z.object({
    url: linkUrlSchema.optional(),
    code: linkCodeSchema.optional(),
    password: z.string().max(48, 'errorsContent.linkPasswordMax').nullish(),
    maxClicks: z.number().int().min(0, 'errorsContent.maxClicksMin').optional(),
    expiration: z.number().min(0, 'errorsContent.expirationMin').nullish(),
});

export default defineEventHandler(async (event) => {
    linkShortenerOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const findLinkById = await prisma.link.findUnique({
        where: { id: getRouterParam(event, 'id'), authorId: currentUser.id },
    });

    if (!findLinkById) {
        throw apiError(404, 'errorsContent.linkNotFound');
    }

    if (body.data.code && body.data.code !== findLinkById.code) {
        await assertLinkCodeFree(body.data.code);
    }

    const link = await prisma.link.update({
        where: { id: findLinkById.id },
        data: {
            url: body.data.url,
            code: body.data.code,
            maxClicks: body.data.maxClicks,
            ...(body.data.password !== undefined && { password: body.data.password || null }),
            ...(body.data.expiration !== undefined && {
                expiresAt: body.data.expiration
                    ? new Date(Date.now() + body.data.expiration)
                    : null,
            }),
        },
    });

    const serialized = serializeLink(event, link, currentUser.domains);

    await createLog(event, {
        action: 'Update Link',
        message: `Updated link ${link.code}`,
    });

    sendToUser(currentUser.id, 'link:update', serialized);

    return serialized;
});
