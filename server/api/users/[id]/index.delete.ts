import { rm } from 'node:fs/promises';

import { join } from 'pathe';
import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const currentUser = event.context.user!;
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

    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data?.verificationData);

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
        await rm(join(uploadsPath, fileName), { force: true }).catch(() => null);
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
        message: `Deleted user ${findUserById.username}`,
    });

    await sendByFilter(isAdmin, 'delete:user', userId);

    sendToUser(findUserById.id, 'logout', null);
});
