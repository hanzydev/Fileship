import { nanoid } from 'nanoid';
import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object({
    destinationUrl: z.url('Invalid destination URL'),
    vanity: z
        .string()
        .max(48, 'Vanity must be at most 48 characters')
        .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, '').trim())
        .nullish(),
    password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
    maxViews: z.number().min(0, 'Max views must be at least 0').nullish(),
    expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
});

export default defineEventHandler(async (event) => {
    urlShortenerOnly(event);

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

    if (body.data.vanity) {
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

    const _url = await prisma.url.create({
        data: {
            vanity: body.data.vanity || nanoid(8),
            destinationUrl: body.data.destinationUrl,
            password: body.data.password || null,
            maxViews: body.data.maxViews || 0,
            expiresAt: body.data.expiration ? new Date(Date.now() + body.data.expiration) : null,
            authorId: currentUser.id,
        },
        include: {
            views: true,
        },
    });

    const url = {
        ..._url,
        views: {
            total: _url.views.length,
            today: _url.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(event, currentUser.domains, `/link/${_url.vanity}`),
    };

    await insert(urlSearchDb, {
        id: url.id,
        vanity: url.vanity,
        destinationUrl: url.destinationUrl,
    });

    await createLog(event, {
        action: 'Shorten URL',
        message: `Shortened URL ${url.vanity}`,
    });

    sendToUser(currentUser.id, 'create:url', url);

    return url;
});
