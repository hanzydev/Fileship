import defu from 'defu';

import { defaultUserAiSettings } from '#shared/utils/constants';
import type { IUserAiSettings } from '#shared/utils/types';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;

    await prisma.session.update({
        where: {
            id: currentUser.currentSessionId,
        },
        data: {
            lastSeen: new Date(),
        },
    });

    const stats = await prisma.user.findUnique({
        where: {
            id: currentUser.id,
        },
        select: {
            _count: {
                select: { files: true, folders: true, notes: true },
            },
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
        backupRestoreState: currentUser.backupRestoreState,
        theme: currentUser.theme,
        aiSettings: defu(currentUser.aiSettings, defaultUserAiSettings) as IUserAiSettings,
        stats: stats!._count,
    };
});
