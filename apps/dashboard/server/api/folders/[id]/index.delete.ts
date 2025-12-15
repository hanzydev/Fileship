import { rm } from 'node:fs/promises';

import { join } from 'pathe';
import z from 'zod';

import { remove } from '@orama/orama';

const validationSchema = z.object({
    deleteFilesToo: z.boolean().optional(),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const folderId = getRouterParam(event, 'id');
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            authorId: currentUser.id,
        },
        include: {
            files: true,
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    await prisma.$transaction(async (tx) => {
        if (body.data.deleteFilesToo) {
            const fileIds = findFolderById.files.map((file) => file.id);

            await tx.view.deleteMany({
                where: {
                    fileId: { in: fileIds },
                },
            });

            await tx.file.deleteMany({
                where: {
                    id: { in: fileIds },
                },
            });
        }

        await tx.folder.delete({
            where: {
                id: folderId,
            },
        });
    });

    if (body.data.deleteFilesToo) {
        const deletionPromises = findFolderById.files.map(async (file) => {
            const rmPromises = [
                rm(join(dataDirectory, 'uploads', file.fileName), {
                    force: true,
                }),
            ];

            if (file.mimeType.startsWith('video/')) {
                rmPromises.push(
                    rm(join(dataDirectory, 'thumbnails', `${file.id}.jpeg`), {
                        force: true,
                    }),
                );
            }

            await Promise.allSettled(rmPromises);

            await remove(fileSearchDb, file.id);

            await createLog(event, {
                action: 'Delete File',
                message: `Deleted file ${file.fileName}`,
            });

            sendToUser(currentUser.id, 'file:delete', file.id);
        });

        await Promise.allSettled(deletionPromises);
    }

    await remove(folderSearchDb, folderId!);

    await createLog(event, {
        action: 'Delete Folder',
        message: `Deleted folder ${findFolderById.name}`,
    });

    sendToUser(currentUser.id, 'folder:delete', folderId);
});
