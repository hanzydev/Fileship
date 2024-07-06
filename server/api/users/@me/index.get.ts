import { defu } from 'defu';

import { defaultEmbed } from '~~/utils/constants';
import type { IEmbed } from '~~/utils/types';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    await prisma.session.update({
        where: {
            id: currentUser.currentSessionId,
        },
        data: {
            lastSeen: new Date(),
        },
    });

    return {
        id: currentUser.id,
        username: currentUser.username,
        avatar: currentUser.avatar,
        permissions: currentUser.permissions,
        createdAt: currentUser.createdAt,
        totpEnabled: currentUser.totpEnabled,
        currentSessionId: currentUser.currentSessionId,
        superAdmin: currentUser.superAdmin,
        embed: defu(currentUser.embed, defaultEmbed) as IEmbed,
    };
});
