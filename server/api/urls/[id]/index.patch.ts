import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';

const validationSchema = z
    .object(
        {
            destinationUrl: z
                .string({
                    invalid_type_error: 'Invalid destination URL',
                    required_error: 'Missing destination URL',
                })
                .url('Invalid destination URL')
                .optional(),
            vanity: z
                .string({
                    invalid_type_error: 'Invalid vanity',
                    required_error: 'Missing vanity',
                })
                .max(32, 'Vanity must be at most 32 characters')
                .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, ''))
                .optional(),
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
                .optional(),
            expiration: z
                .number({
                    invalid_type_error: 'Invalid expiration',
                    required_error: 'Missing expiration',
                })
                .min(0, 'Expiration must be at least 0')
                .nullish(),
        },
        { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
    )
    .strict({
        message: 'Body contains unexpected keys',
    });

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
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

    const urlId = getRouterParam(event, 'id');

    const findUrlById = await prisma.url.findUnique({
        where: {
            id: urlId,
        },
    });

    if (!findUrlById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'URL not found',
        });
    }

    if (findUrlById.authorId !== currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You do not have permission to perform this action',
        });
    }

    if ('expiration' in body.data) {
        if (body.data.expiration) {
            (body.data as any).expiresAt = new Date(
                Date.now() + body.data.expiration,
            );
        } else (body.data as any).expiresAt = null;

        delete body.data.expiration;
    }

    let updatedUrl = await prisma.url.update({
        where: {
            id: urlId,
        },
        include: {
            views: true,
        },
        data: body.data,
    });

    updatedUrl = {
        ...updatedUrl,
        views: {
            total: updatedUrl.views.length,
            today: updatedUrl.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
    } as never;

    await createLog(event, {
        action: 'Update URL',
        message: `Updated URL ${findUrlById.vanity}`,
    });

    sendToUser(currentUser.id, 'update:url', updatedUrl);

    return { ...updatedUrl, password: undefined };
});
