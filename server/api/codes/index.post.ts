import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object(
    {
        title: z
            .string({
                invalid_type_error: 'Invalid title',
                required_error: 'Missing title',
            })
            .min(3, 'Title must be at least 3 characters')
            .max(48, 'Title must be at most 48 characters')
            .transform((title) => title.trim()),
        code: z
            .string({
                invalid_type_error: 'Invalid code',
                required_error: 'Missing code',
            })
            .min(5, 'Code must be at least 5 characters')
            .max(100_000, 'Code must be at most 100000 characters'),
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
    codeSharerOnly(event);

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

    const _code = await prisma.code.create({
        data: {
            title: body.data.title,
            code: body.data.code,
            password: body.data.password || null,
            maxViews: body.data.maxViews || 0,
            expiresAt: body.data.expiration ? new Date(Date.now() + body.data.expiration) : null,
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
        url: buildPublicUrl(event, currentUser.domains, `/code/${_code.id}`),
    };

    await insert(codeSearchDb, {
        id: code.id,
        title: code.title,
        language: code.language,
    });

    await createLog(event, {
        action: 'Share Code',
        message: `Shared code ${code.title}`,
    });

    sendToUser(currentUser.id, 'create:code', code);

    return code;
});
