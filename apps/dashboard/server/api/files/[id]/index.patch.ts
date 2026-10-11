import { join } from 'pathe';
import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        fileName: z
            .string()
            .transform((value) => value.replace(/[^a-zA-Z0-9-_.]/g, '').trim())
            .pipe(
                z
                    .string()
                    .min(3, 'File name must be at least 3 characters')
                    .max(255, 'File name must be at most 255 characters')
                    .regex(/^[^.]/, 'File name cannot start with a dot'),
            )
            .optional(),
        password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
        maxViews: z.number().min(0, 'Max views must be at least 0').optional(),
        expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
        folderId: z.string().nullish(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const fileId = getRouterParam(event, 'id');

    const findFileById = await prisma.file.findUnique({
        where: {
            id: fileId,
            authorId: currentUser.id,
        },
    });

    if (!findFileById) {
        throw createError({
            status: 404,
            message: 'File not found',
        });
    }

    if ('expiration' in body.data) {
        if (body.data.expiration) {
            (body.data as any).expiresAt = new Date(Date.now() + body.data.expiration);
        } else (body.data as any).expiresAt = null;

        delete body.data.expiration;
    }

    if (body.data.folderId) {
        const findFolderById = await prisma.folder.findUnique({
            where: {
                id: body.data.folderId,
                authorId: currentUser.id,
            },
        });

        if (!findFolderById) {
            throw createError({
                status: 404,
                message: 'Folder not found',
            });
        }
    }

    if (body.data.fileName && body.data.fileName !== findFileById.fileName) {
        const findFileByFileName = await prisma.file.findUnique({
            where: {
                fileName: body.data.fileName,
            },
        });

        if (findFileByFileName) {
            throw createError({
                status: 409,
                message: 'A file with that name already exists',
            });
        }

        await moveFileRobust(
            join(dataDirectory, 'uploads', findFileById.fileName),
            join(dataDirectory, 'uploads', body.data.fileName),
        );
    }

    const _updatedFile = await prisma.file.update({
        where: {
            id: fileId,
        },
        include: {
            folder: { select: { id: true, name: true } },
            _count: { select: { views: true } },
        },
        data: body.data,
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
        },
    });

    const todayViews = await getTodayViews([_updatedFile.id]);
    const updatedFile = serializeFile(
        event,
        _updatedFile,
        currentUser.domains,
        todayViews.get(_updatedFile.id),
    );

    await update(fileSearchDb, updatedFile.id, {
        id: updatedFile.id,
        authorId: updatedFile.authorId,
        fileName: updatedFile.fileName,
        mimeType: updatedFile.mimeType,
        embedding: (findFileById.embedding as never[]) || undefined,
        textEmbedding: (findFileById.textEmbedding as never[]) || undefined,
        caption: findFileById.caption || undefined,
        ocrText: findFileById.ocrText || undefined,
    });

    await createLog(event, {
        action: 'Update File',
        message: `Updated file ${updatedFile.fileName}`,
    });

    sendToUser(
        currentUser.id,
        'file:update',
        updatedFile.folderId === findFileById.folderId
            ? { ...updatedFile, folderId: undefined, folder: undefined }
            : updatedFile,
    );

    return updatedFile;
});
