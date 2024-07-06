import { nanoid } from 'nanoid';
import { z } from 'zod';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { canShortenUrls, isAdmin } from '~~/utils/user';

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
            .max(32, 'Vanity must be at most 32 characters')
            .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, ''))
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
    const currentUser = event.context.user!;
    if (!canShortenUrls(currentUser)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
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

    let url = await prisma.url.create({
        data: {
            vanity: body.data.vanity || nanoid(8),
            destinationUrl: body.data.destinationUrl,
            password: body.data.password || null,
            maxViews: body.data.maxViews || 0,
            expiresAt: body.data.expiration
                ? new Date(Date.now() + body.data.expiration)
                : null,
            authorId: currentUser.id,
        },
        include: {
            views: true,
        },
    });

    url = {
        ...url,
        views: {
            total: url.views.length,
            today: url.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
    } as never;

    const log = await prisma.log.create({
        data: {
            action: 'Shorten URL',
            userId: currentUser.id,
            message: `Shortened url ${url.vanity}`,
            ip: getRequestIP(event, { xForwardedFor: true })!,
        },
        include: {
            user: {
                select: {
                    id: true,
                    username: true,
                },
            },
        },
    });

    sendToUser(currentUser.id, 'create:url', url);
    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );

    return {
        ...url,
        url: `${getRequestURL(event).origin}/link/${url.vanity}`,
    };
});
