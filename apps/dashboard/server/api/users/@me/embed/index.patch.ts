import defu from 'defu';
import { z } from 'zod';

const validationSchema = z
    .object({
        title: z.string().optional(),
        description: z.string().optional(),
        siteName: z.string().optional(),
        color: z
            .string()
            .startsWith('#', 'Color must start with #')
            .length(7, 'Color must be 7 characters')
            .optional(),
        enabled: z.boolean(),
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

    const embed = defu(body.data, currentUser.embed, defaultEmbed);

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            embed,
        },
    });

    sendToUser(currentUser.id, 'update:embed', embed);

    return embed as IEmbed;
});
