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
            status: 400,
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
            inbox: true,
        },
    });

    if (!findFolderById) {
        throw createError({
            status: 404,
            message: 'Folder not found',
        });
    }

    if (findFolderById.inbox) {
        throw createError({
            status: 400,
            message: 'Cannot delete inbox folder',
        });
    }

    await prisma.$transaction(async (tx) => {
        if (body.data.deleteFilesToo) {
            const fileIds = findFolderById.files.map((file) => file.id);

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
        await removeFileData(findFolderById.files);

        const deletionPromises = findFolderById.files.map(async (file) => {
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
