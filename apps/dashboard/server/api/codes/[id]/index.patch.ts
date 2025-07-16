import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        title: z
            .string()
            .min(3, 'Title must be at least 3 characters')
            .max(48, 'Title must be at most 48 characters')
            .optional()
            .transform((title) => title?.trim()),
        code: z
            .string()
            .min(5, 'Code must be at least 5 characters')
            .max(100_000, 'Code must be at most 100000 characters')
            .optional(),
        language: z
            .string()
            .min(1, 'Language must be at least 1 character')
            .max(32, 'Language must be at most 32 characters')
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
        data: body.data,
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
