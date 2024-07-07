import { rm } from 'node:fs/promises';

import { join } from 'pathe';

import { sendByFilter, sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user!;
    if (!isAdmin(currentUser)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    const userId = getRouterParam(event, 'id');

    if (userId === currentUser.id) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You cannot delete yourself',
        });
    }

    const findUserById = await prisma.user.findUnique({
        where: {
            id: userId,
        },
    });

    if (!findUserById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'User not found',
        });
    }

    if (findUserById.superAdmin && !currentUser.superAdmin) {
        throw createError({
            statusCode: 403,
            statusMessage: 'Forbidden',
            message: 'You cannot delete a super admin',
        });
    }

    const userFiles = await prisma.file.findMany({
        where: {
            authorId: userId,
        },
        select: {
            fileName: true,
        },
    });

    const uploadsPath = join(dataDirectory, 'uploads');

    for (const { fileName } of userFiles) {
        await rm(join(uploadsPath, fileName), { force: true });
    }

    await prisma.$transaction([
        prisma.view.deleteMany({
            where: {
                OR: [
                    {
                        file: {
                            authorId: userId,
                        },
                    },
                    {
                        code: {
                            authorId: userId,
                        },
                    },
                    {
                        url: {
                            authorId: userId,
                        },
                    },
                ],
            },
        }),
        prisma.folder.deleteMany({
            where: {
                authorId: userId,
            },
        }),
        prisma.note.deleteMany({
            where: {
                authorId: userId,
            },
        }),
        prisma.code.deleteMany({
            where: {
                authorId: userId,
            },
        }),
        prisma.url.deleteMany({
            where: {
                authorId: userId,
            },
        }),
        prisma.file.deleteMany({
            where: {
                authorId: userId,
            },
        }),
        prisma.session.deleteMany({
            where: {
                userId,
            },
        }),
        prisma.user.delete({
            where: {
                id: userId,
            },
        }),
    ]);

    await createLog(event, {
        action: 'Delete User',
        message: `Deleted ${findUserById.username}`,
    });

    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'delete:user',
        userId,
    );

    sendToUser(findUserById.id, 'logout', null);
});
