import { z } from 'zod';

const validationSchema = z.object({
    code: z.string(),
    password: z.string().max(48, 'errorsContent.linkPasswordMax'),
});

export default defineEventHandler(async (event) => {
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const link = await findActiveLink(body.data.code);

    if (!(await limitPasswordAttempts(event, link.id, link.password === body.data.password))) {
        throw apiError(401, 'errorsContent.invalidPassword');
    }

    await countLinkClick(event, link);

    return { url: link.url };
});
