import { rm } from 'node:fs/promises';

import { join } from 'pathe';
import z from 'zod';

import { removeMultiple } from '@orama/orama';

const validationSchema = z.object({
    files: z.array(z.uuid()).min(1),
});

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

    const filesToBeDeleted = await prisma.file.findMany({
        where: {
            id: { in: body.data.files },
            authorId: currentUser.id,
        },
        select: {
            id: true,
            fileName: true,
            mimeType: true,
        },
    });

    if (!filesToBeDeleted.length) {
        throw createError({
            statusCode: 404,
            message: 'No files found to delete',
        });
    }

    const fileIds = filesToBeDeleted.map((file) => file.id);

    await prisma.$transaction(async (tx) => {
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
    });

    const deletionPromises = filesToBeDeleted.map(async (file) => {
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
    });

    await Promise.allSettled(deletionPromises);

    await removeMultiple(fileSearchDb, fileIds);

    sendToUser(currentUser.id, 'file:bulkDelete', fileIds);

    await createLog(event, {
        action: 'Bulk Delete Files',
        message: `Deleted ${fileIds.length} files`,
    });
});
