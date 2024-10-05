import { randomUUID } from 'node:crypto';
import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import { nanoid } from 'nanoid';
import { extname, join } from 'pathe';
import sharp from 'sharp';
import { z } from 'zod';

import { sendToUser } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/permissions';

const validationSchema = z.object(
    {
        totalChunks: z
            .number({
                invalid_type_error: 'Invalid total chunks',
            })
            .min(1, 'Total chunks must be at least 1')
            .optional(),
        currentChunk: z
            .number({
                invalid_type_error: 'Invalid current chunk',
            })
            .min(1, 'Current chunk must be at least 1')
            .optional(),
        fileNameType: z
            .union(
                [z.literal('Random'), z.literal('UUID'), z.literal('Original')],
                {
                    invalid_type_error:
                        'File name type must be Random, UUID, or Original',
                },
            )
            .nullish(),
        compression: z
            .number({
                invalid_type_error: 'Invalid compression',
            })
            .min(0, 'Compression must be at least 0')
            .max(100, 'Compression must be at most 100')
            .optional(),
        password: z
            .string({
                invalid_type_error: 'Invalid password',
            })
            .max(48, 'Password must be at most 48 characters')
            .nullish(),
        maxViews: z
            .number({
                invalid_type_error: 'Invalid max views',
            })
            .min(0, 'Max views must be at least 0')
            .nullish(),
        expiration: z
            .number({
                invalid_type_error: 'Invalid expiration',
            })
            .min(0, 'Expiration must be at least 0')
            .nullish(),
        folderId: z
            .string({
                invalid_type_error: 'Invalid folder id',
            })
            .nullish(),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const file = formData.get('file');
    if (!file) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Missing file',
        });
    }

    if (typeof file === 'string') {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid file',
        });
    }

    const body = validationSchema.safeParse({
        fileNameType: formData.get('fileNameType'),
        currentChunk: +formData.get('currentChunk')! || 1,
        totalChunks: +formData.get('totalChunks')! || 1,
        maxViews: +formData.get('maxViews')!,
        password: formData.get('password') as string,
        expiration: +formData.get('expiration')!,
        compression: +formData.get('compression')!,
        folderId: formData.get('folderId') as string,
    });

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
            message: 'Invalid body',
            data: body.error.format(),
        });
    }

    let fileName = file.name.replace(/[^a-zA-Z0-9-_.]/g, '');

    if (body.data.fileNameType === 'Random') {
        fileName = `${nanoid(8)}${extname(file.name)}`;
    } else if (body.data.fileNameType === 'UUID') {
        fileName = `${randomUUID()}${extname(file.name)}`;
    }

    const findFileByFileName = await prisma.file.findUnique({
        where: {
            fileName: fileName,
        },
    });

    if (findFileByFileName) {
        throw createError({
            statusCode: 409,
            statusMessage: 'Conflict',
            message: 'A file with that name already exists',
        });
    }

    if (body.data.folderId) {
        const findFolderById = await prisma.folder.findUnique({
            where: {
                id: body.data.folderId,
            },
        });

        if (!findFolderById) {
            throw createError({
                statusCode: 404,
                statusMessage: 'Not Found',
                message: 'Folder not found',
            });
        }
    }

    const tempPath = join(
        dataDirectory,
        'temp',
        currentUser.id,
        file.name.replace(/[^a-zA-Z0-9-_.]/g, ''),
    );

    if (!existsSync(join(dataDirectory, 'temp', currentUser.id))) {
        await fsp.mkdir(join(dataDirectory, 'temp', currentUser.id));
    }

    const buffer = new Uint8Array(await file.arrayBuffer());

    if (body.data.currentChunk === body.data.totalChunks) {
        if (existsSync(tempPath)) {
            await fsp.appendFile(tempPath, buffer);
        } else {
            await fsp.writeFile(tempPath, buffer);
        }

        if (file.type.startsWith('image/') && file.type !== 'image/gif') {
            try {
                const temp2Path = join(
                    dataDirectory,
                    'temp',
                    currentUser.id,
                    `${nanoid(8)}${extname(file.name)}`,
                );

                const image = sharp(tempPath);
                const metadata = await image.metadata();

                const removeExifData =
                    (process.env.REMOVE_EXIF_DATA || 'true') === 'true';

                if (
                    metadata.width &&
                    metadata.height &&
                    body.data.compression
                ) {
                    await image
                        .jpeg({
                            quality: Math.min(
                                100,
                                Math.max(1, 100 - body.data.compression),
                            ),
                        })
                        .toFile(temp2Path);
                    await fsp.rename(temp2Path, tempPath);
                } else if (removeExifData) {
                    await image.rotate().toFile(temp2Path);
                    await fsp.rename(temp2Path, tempPath);
                }
            } catch {
                throw createError({
                    statusCode: 400,
                    statusMessage: 'Bad Request',
                    message: 'Failed to compress image',
                });
            }
        }

        const fileSize = (await fsp.stat(tempPath)).size;
        const totalSize = (await fsp.stat(join(dataDirectory, 'uploads'))).size;

        if (
            !isAdmin(currentUser) &&
            currentUser.limits.usableSpace > -1 &&
            totalSize + fileSize > currentUser.limits.usableSpace * 1024 * 1024
        ) {
            throw createError({
                statusCode: 400,
                statusMessage: 'Bad Request',
                message: 'Storage limit reached',
            });
        }

        const filePath = join(dataDirectory, 'uploads', fileName);

        await fsp.rename(tempPath, filePath);

        const _upload = await prisma.file.create({
            data: {
                fileName,
                mimeType: file.type,
                size: fileSize,
                password: body.data.password || null,
                maxViews: body.data.maxViews || 0,
                expiresAt: body.data.expiration
                    ? new Date(Date.now() + body.data.expiration)
                    : null,
                authorId: currentUser.id,
                folderId: body.data.folderId || null,
            },
            include: {
                views: true,
            },
        });

        const upload = {
            ..._upload,
            size: {
                raw: _upload.size.toString(),
                formatted: filesize(_upload.size.toString()),
            },
            views: {
                total: _upload.views.length,
                today: _upload.views.filter((view) => {
                    const now = new Date();

                    return (
                        view.createdAt.getDate() === now.getDate() &&
                        view.createdAt.getMonth() === now.getMonth() &&
                        view.createdAt.getFullYear() === now.getFullYear()
                    );
                }).length,
            },
            directUrl: buildPublicUrl(
                event,
                currentUser.domains,
                `/u/${_upload.fileName}`,
            ),
            embedUrl: buildPublicUrl(
                event,
                currentUser.domains,
                `/view/${_upload.fileName}`,
            ),
            url: buildPublicUrl(
                event,
                currentUser.domains,
                `/${currentUser.embed.enabled ? 'view' : 'u'}/${_upload.fileName}`,
            ),
        };

        await createLog(event, {
            action: 'Upload File',
            message: `Uploaded file ${fileName}`,
        });

        sendToUser(currentUser.id, 'create:file', upload);
        sendToUser(currentUser.id, 'folder:file:add', {
            folderId: body.data.folderId,
            fileId: upload.id,
        });

        return upload;
    } else if (body.data.currentChunk === 1) {
        await fsp.writeFile(tempPath, buffer);
    } else {
        await fsp.appendFile(tempPath, buffer);
    }
});
