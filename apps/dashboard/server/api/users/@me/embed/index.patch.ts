import { defu } from 'defu';
import { z } from 'zod';

const validationSchema = z
    .object({
        title: z.string().optional(),
        description: z.string().optional(),
        siteName: z.string().optional(),
        color: z
            .string()
            .startsWith('#', 'errorsUsers.colorStart')
            .length(7, 'errorsUsers.colorLength')
            .optional(),
        enabled: z.boolean(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
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

    sendToUser(currentUser.id, 'currentUser:embedUpdate', embed);

    return embed as IEmbed;
});
