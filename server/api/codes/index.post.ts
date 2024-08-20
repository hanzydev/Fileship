import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';
import { canShareCodes } from '~~/utils/user';

const validationSchema = z.object(
    {
        title: z
            .string({
                invalid_type_error: 'Invalid title',
                required_error: 'Missing title',
            })
            .min(1, 'Title must be at least 1 character')
            .max(32, 'Title must be at most 32 characters'),
        code: z
            .string({
                invalid_type_error: 'Invalid code',
                required_error: 'Missing code',
            })
            .min(1, 'Code must be at least 1 character')
            .max(50000, 'Code must be at most 50000 characters'),
        language: z
            .string({
                invalid_type_error: 'Invalid language',
                required_error: 'Missing language',
            })
            .min(1, 'Language must be at least 1 character')
            .max(32, 'Language must be at most 32 characters'),
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
    if (!canShareCodes(currentUser)) {
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

    const _code = await prisma.code.create({
        data: {
            title: body.data.title,
            code: body.data.code,
            password: body.data.password || null,
            maxViews: body.data.maxViews || 0,
            expiresAt: body.data.expiration
                ? new Date(Date.now() + body.data.expiration)
                : null,
            language: body.data.language,
            authorId: currentUser.id,
        },
        include: {
            views: true,
        },
    });

    const code = {
        ..._code,
        views: {
            total: _code.views.length,
            today: _code.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
    };

    await createLog(event, {
        action: 'Share Code',
        message: `Shared code ${code.title}`,
    });

    sendToUser(currentUser.id, 'create:code', code);

    return code;
});
