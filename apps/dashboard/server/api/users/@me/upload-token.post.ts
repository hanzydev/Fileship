import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data?.verificationData);

    const token = await generateUploadToken(event.context.user!.id);

    await createLog(event, {
        action: 'Regenerate Upload Token',
        message: 'Regenerated the upload token',
    });

    return { token };
});
