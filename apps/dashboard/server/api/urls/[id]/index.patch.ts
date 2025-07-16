import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        destinationUrl: z.url('Invalid destination URL').optional(),
        vanity: z
            .string()
            .min(3, 'Vanity must be at least 3 characters')
            .max(48, 'Vanity must be at most 48 characters')
            .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, '').trim())
            .optional(),
        password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
        maxViews: z.number().min(0, 'Max views must be at least 0').optional(),
        expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const urlId = getRouterParam(event, 'id');

    const findUrlById = await prisma.url.findUnique({
        where: {
            id: urlId,
            authorId: currentUser.id,
        },
    });

    if (!findUrlById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'URL not found',
        });
    }

    if ('expiration' in body.data) {
        if (body.data.expiration) {
            (body.data as any).expiresAt = new Date(Date.now() + body.data.expiration);
        } else (body.data as any).expiresAt = null;

        delete body.data.expiration;
    }

    if (body.data.vanity && body.data.vanity !== findUrlById.vanity) {
        const findUrlByVanity = await prisma.url.findUnique({
            where: {
                vanity: body.data.vanity,
            },
        });

        if (findUrlByVanity) {
            throw createError({
                statusCode: 409,
                statusMessage: 'Conflict',
                message: 'A URL with that vanity already exists',
            });
        }
    }

    const _updatedUrl = await prisma.url.update({
        where: {
            id: urlId,
        },
        include: {
            views: true,
        },
        data: body.data,
    });

    const updatedUrl = {
        ..._updatedUrl,
        views: {
            total: _updatedUrl.views.length,
            today: _updatedUrl.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(event, currentUser.domains, `/link/${_updatedUrl.vanity}`),
    };

    await update(urlSearchDb, updatedUrl.id, {
        id: updatedUrl.id,
        vanity: updatedUrl.vanity,
        destinationUrl: updatedUrl.destinationUrl,
    });

    await createLog(event, {
        action: 'Update URL',
        message: `Updated URL ${findUrlById.vanity}`,
    });

    sendToUser(currentUser.id, 'update:url', updatedUrl);

    return updatedUrl;
});
