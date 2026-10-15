import { z } from 'zod';

const validationSchema = z
    .object({
        enabled: z.boolean().optional(),
        slug: z
            .string()
            .min(1)
            .max(48, 'Slug must be at most 48 characters')
            .transform((value) =>
                value
                    .toLowerCase()
                    .replace(/[^a-z0-9-_]/g, '')
                    .trim(),
            )
            .pipe(
                z
                    .string()
                    .min(1, 'Slug can only contain letters, numbers, dashes and underscores')
                    .refine((value) => !isUuid(value), 'This slug is reserved'),
            )
            .nullish(),
        password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (body.data.slug) {
        const findInboxBySlug = await prisma.inbox.findFirst({
            where: {
                slug: body.data.slug,
                userId: { not: currentUser.id },
            },
        });

        if (findInboxBySlug) {
            throw createError({
                status: 409,
                message: 'Slug already taken',
            });
        }
    }

    await ensureInbox(currentUser.id);

    await prisma.inbox.update({
        where: {
            userId: currentUser.id,
        },
        data: body.data,
    });
});
