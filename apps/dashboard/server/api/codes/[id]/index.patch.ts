import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object(
        {
            title: z
                .string({
                    invalid_type_error: 'Invalid title',
                    required_error: 'Missing title',
                })
                .min(3, 'Title must be at least 3 characters')
                .max(48, 'Title must be at most 48 characters')
                .optional()
                .transform((title) => title?.trim()),
            code: z
                .string({
                    invalid_type_error: 'Invalid code',
                    required_error: 'Missing code',
                })
                .min(5, 'Code must be at least 5 characters')
                .max(100_000, 'Code must be at most 100000 characters')
                .optional(),
            language: z
                .string({
                    invalid_type_error: 'Invalid language',
                    required_error: 'Missing language',
                })
                .min(1, 'Language must be at least 1 character')
                .max(32, 'Language must be at most 32 characters')
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

    const codeId = getRouterParam(event, 'id');

    const findCodeById = await prisma.code.findUnique({
        where: {
            id: codeId,
            authorId: currentUser.id,
        },
    });

    if (!findCodeById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Code not found',
        });
    }

    if ('expiration' in body.data) {
        if (body.data.expiration) {
            (body.data as any).expiresAt = new Date(Date.now() + body.data.expiration);
        } else (body.data as any).expiresAt = null;

        delete body.data.expiration;
    }

    const _updatedCode = await prisma.code.update({
        where: {
            id: codeId,
        },
        include: {
            views: true,
        },
        data: {
            title: body.data.title || findCodeById.title,
            ...body.data,
        },
    });

    const updatedCode = {
        ..._updatedCode,
        views: {
            total: _updatedCode.views.length,
            today: _updatedCode.views.filter((view) => {
                const now = new Date();

                return (
                    view.createdAt.getDate() === now.getDate() &&
                    view.createdAt.getMonth() === now.getMonth() &&
                    view.createdAt.getFullYear() === now.getFullYear()
                );
            }).length,
        },
        url: buildPublicUrl(event, currentUser.domains, `/code/${_updatedCode.id}`),
    };

    await update(codeSearchDb, _updatedCode.id, {
        id: updatedCode.id,
        title: updatedCode.title,
        language: updatedCode.language,
    });

    await createLog(event, {
        action: 'Update Code',
        message: `Updated code ${findCodeById.title}`,
    });

    sendToUser(currentUser.id, 'update:code', updatedCode);

    return updatedCode;
});
