import { z } from 'zod';

const validationSchema = z
    .object({
        enabled: z.boolean().optional(),
        slug: z
            .string()
            .min(1, 'errorsUsers.slugRequired')
            .max(48, 'errorsUsers.slugMax')
            .transform((value) =>
                value
                    .toLowerCase()
                    .replace(/[^a-z0-9-_]/g, '')
                    .trim(),
            )
            .pipe(
                z
                    .string()
                    .min(1, 'errorsUsers.slugCharacters')
                    .refine((value) => !isUuid(value), 'errorsUsers.slugReserved'),
            )
            .nullish(),
        password: z.string().max(48, 'errorsUsers.passwordMax').nullish(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    if (body.data.slug) {
        const findInboxBySlug = await prisma.inbox.findFirst({
            where: {
                slug: body.data.slug,
                userId: { not: currentUser.id },
            },
        });

        if (findInboxBySlug) {
            throw apiError(409, 'errorsUsers.slugTaken');
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
