import { defu } from 'defu';
import { z } from 'zod';

const validationSchema = z
    .object({
        enabled: z.boolean().optional(),
        suppressPii: z.boolean().optional(),
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

    const aiSettings = defu(body.data, currentUser.aiSettings, defaultUserAiSettings);

    await prisma.user.update({
        where: { id: currentUser.id },
        data: { aiSettings },
    });

    sendToUser(currentUser.id, 'currentUser:update', { aiSettings });

    return aiSettings;
});
