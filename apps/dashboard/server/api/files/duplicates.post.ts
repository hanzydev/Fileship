import z from 'zod';

const validationSchema = z.object({
    files: z
        .array(
            z.object({
                size: z.number().int().min(0),
                hash: z
                    .string()
                    .regex(/^[0-9a-f]{64}$/)
                    .optional(),
            }),
        )
        .min(1)
        .max(100),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    return { files: await checkUploads(event, event.context.user!.id, body.data.files) };
});
