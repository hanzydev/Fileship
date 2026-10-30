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
        discord: discordSettingsSchema.optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const embed = embedOf({ ...embedOf(currentUser.embed), ...body.data });

    for (const kind of DISCORD_EMBED_KINDS) {
        const layout = embed.discord[kind];

        if (layout && !serializeDiscordEmbed(layout, DISCORD_SAMPLE_VALUES, embed.color)) {
            throw apiError(400, 'errorsUsers.discordLayoutInvalid', undefined, {
                discordLayoutInvalid: kind,
            });
        }
    }

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            embed: embed as any,
        },
    });

    sendToUser(currentUser.id, 'currentUser:embedUpdate', embed);

    return embed;
});
