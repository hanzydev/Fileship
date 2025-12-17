import { randomUUID } from 'node:crypto';
import { existsSync, promises as fsp } from 'node:fs';

import { filesize } from 'filesize';
import fluentFfmpeg from 'fluent-ffmpeg';
import { nanoid } from 'nanoid';
import { basename, extname, join } from 'pathe';
import sharp from 'sharp';
import { z } from 'zod';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import { insert } from '@orama/orama';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

const validationSchema = z.object({
    totalChunks: z.number().min(1, 'Total chunks must be at least 1').optional(),
    currentChunk: z.number().min(1, 'Current chunk must be at least 1').optional(),
    fileNameType: z
        .union([z.literal('Random'), z.literal('UUID'), z.literal('Original')])
        .nullish(),
    compression: z
        .number()
        .min(0, 'Compression must be at least 0')
        .max(100, 'Compression must be at most 100')
        .optional(),
    password: z.string().max(48, 'Password must be at most 48 characters').nullish(),
    maxViews: z.number().min(0, 'Max views must be at least 0').nullish(),
    expiration: z.number().min(0, 'Expiration must be at least 0').nullish(),
    folderId: z.string().nullish(),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const formData = await readFormData(event);

    const file = formData.get('file');
    if (!file) {
        throw createError({
            statusCode: 400,
            message: 'Missing file',
        });
    }

    if (typeof file === 'string') {
        throw createError({
            statusCode: 400,
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
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const compressible =
        file.type.startsWith('image/') && file.type !== 'image/gif' && body.data.compression;

    const extensionName = compressible ? '.jpeg' : extname(file.name);

    let fileName = `${basename(file.name.replace(/[^a-zA-Z0-9-_.]/g, ''), extname(file.name))}${
        extensionName
    }`;

    if (body.data.fileNameType === 'Random') {
        fileName = `${nanoid(8)}${extensionName}`;
    } else if (body.data.fileNameType === 'UUID') {
        fileName = `${randomUUID()}${extensionName}`;
    }

    const findFileByFileName = await prisma.file.findUnique({
        where: {
            fileName: fileName,
        },
    });

    if (findFileByFileName) {
        throw createError({
            statusCode: 409,
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

    const removeExifData = (process.env.REMOVE_EXIF_DATA || 'true') === 'true';

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

                if (metadata.width && metadata.height && body.data.compression) {
                    await image
                        .jpeg({
                            quality: Math.min(100, Math.max(1, 100 - body.data.compression)),
                        })
                        .toFile(temp2Path);
                    await fsp.rename(temp2Path, tempPath);
                } else if (removeExifData) {
                    await image.rotate().toFile(temp2Path);
                    await fsp.rename(temp2Path, tempPath);
                }
            } catch {
                //
            }
        }

        const fileSize = (await fsp.stat(tempPath)).size;
        const totalSize = await prisma.file.aggregate({
            where: {
                authorId: currentUser.id,
            },
            _sum: {
                size: true,
            },
        });

        if (
            !isAdmin(currentUser) &&
            currentUser.limits.usableSpace > -1 &&
            (totalSize._sum.size ?? 0n) + BigInt(fileSize) >
                BigInt(currentUser.limits.usableSpace * 1024 * 1024)
        ) {
            throw createError({
                statusCode: 400,
                message: 'Storage limit reached',
            });
        }

        const filePath = join(dataDirectory, 'uploads', fileName);

        await fsp.rename(tempPath, filePath);

        const _upload = await prisma.file.create({
            data: {
                fileName,
                mimeType: compressible ? 'image/jpeg' : file.type,
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
            omit: {
                embedding: true,
            },
        });

        const upload = {
            ..._upload,
            size: {
                raw: _upload.size.toString(),
                formatted: filesize(_upload.size),
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
            directUrl: buildPublicUrl(event, currentUser.domains, `/u/${_upload.fileName}`),
            embedUrl: buildPublicUrl(event, currentUser.domains, `/view/${_upload.fileName}`),
            thumbnailUrl: undefined as string | null | undefined,
        };

        if (file.type.startsWith('video/')) {
            const thumbnailPath = join(dataDirectory, 'thumbnails', `${upload.id}.jpeg`);

            await new Promise<void>((resolve) => {
                fluentFfmpeg(filePath)
                    .videoFilters('thumbnail')
                    .frames(1)
                    .format('mjpeg')
                    .output(thumbnailPath)
                    .on('end', () => resolve())
                    .on('error', () => resolve())
                    .run();
            });

            upload.thumbnailUrl = existsSync(thumbnailPath)
                ? buildPublicUrl(event, currentUser.domains, `/u/${_upload.fileName}/thumbnail`)
                : null;
        }

        let embedding: number[] | undefined = undefined;

        if (IMAGE_EMBEDDING_SUPPORTED_EXTENSIONS.includes(extensionName)) {
            const clip = await getClipInstance();
            embedding = await clip.createImageEmbedding(filePath);

            await prisma.file.update({
                where: {
                    id: upload.id,
                },
                data: {
                    embedding,
                },
            });
        }

        await insert(fileSearchDb, {
            id: upload.id,
            fileName: upload.fileName,
            mimeType: upload.mimeType,
            embedding,
        });

        await createLog(event, {
            action: 'Upload File',
            message: `Uploaded file ${fileName}`,
        });

        sendToUser(currentUser.id, 'file:create', upload);

        return {
            ...upload,
            url: buildPublicUrl(
                event,
                currentUser.domains,
                `/${currentUser.embed.enabled ? 'view' : 'u'}/${_upload.fileName}`,
            ),
        };
    } else if (body.data.currentChunk === 1) {
        await fsp.writeFile(tempPath, buffer);
    } else {
        await fsp.appendFile(tempPath, buffer);
    }
});
