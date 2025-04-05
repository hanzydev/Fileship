import { nanoid } from 'nanoid';
import { z } from 'zod';

const validationSchema = z.object(
    {
        destinationUrl: z
            .string({
                invalid_type_error: 'Invalid destination URL',
                required_error: 'Missing destination URL',
            })
            .url('Invalid destination URL'),
        vanity: z
            .string({
                invalid_type_error: 'Invalid vanity',
                required_error: 'Missing vanity',
            })
            .max(48, 'Vanity must be at most 48 characters')
            .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, '').trim())
            .nullish(),
        password: z
            .string({
                invalid_type_error: 'Invalid password',
                required_error: 'Missing password',
            })
            .max(48, 'Password must be at most 48 characters')
            .nullish(),
        maxViews: z
            .number({
                invalid_type_error: 'Invalid max views',
                required_error: 'Missing max views',
            })
            .min(0, 'Max views must be at least 0')
            .nullish(),
        expiration: z
            .number({
                invalid_type_error: 'Invalid expiration',
                required_error: 'Missing expiration',
            })
            .min(0, 'Expiration must be at least 0')
            .nullish(),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

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

    await createLog(event, {
        action: 'Shorten URL',
        message: `Shortened URL ${url.vanity}`,
    });

    sendToUser(currentUser.id, 'create:url', url);

    return url;
});
