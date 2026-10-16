import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data?.verificationData);

    const files = await resetAIData();

    await createLog(event, {
        action: 'Reset AI Data',
        message: `Reset the AI data of ${files} files`,
    });

    return { files };
});
