import { existsSync, promises as fsp } from 'node:fs';

import fluentFfmpeg from 'fluent-ffmpeg';
import type { H3Event } from 'h3';
import { join } from 'pathe';
import sharp from 'sharp';

import ffmpeg from '@ffmpeg-installer/ffmpeg';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

export const getThumbnailPath = (id: string) => join(dataDirectory, 'thumbnails', `${id}.jpeg`);

export const hasThumbnail = (id: string) => existsSync(getThumbnailPath(id));

export const canHaveThumbnail = (mimeType: string) =>
    mimeType.startsWith('video/') ||
    (mimeType.startsWith('image/') && !['image/gif', 'image/svg+xml'].includes(mimeType));

export const getThumbnailUrl = (
    event: H3Event,
    domains: string[],
    file: { id: string; fileName: string; mimeType: string },
) => {
    if (!canHaveThumbnail(file.mimeType)) return undefined;

    return hasThumbnail(file.id)
        ? buildPublicUrl(event, domains, `/u/${file.fileName}/thumbnail`)
        : null;
};

const createVideoThumbnail = (filePath: string, outputPath: string) =>
    new Promise<void>((resolve) => {
        const command = fluentFfmpeg(filePath)
            .inputOptions('-threads 1')
            .seekInput(1)
            .videoFilters('scale=640:-2')
            .frames(1)
            .format('mjpeg')
            .outputOptions('-threads 1')
            .output(outputPath);

        const timeout = setTimeout(() => {
            command.kill('SIGKILL');
            resolve();
        }, 30_000);

        command
            .on('end', () => {
                clearTimeout(timeout);
                resolve();
            })
            .on('error', () => {
                clearTimeout(timeout);
                resolve();
            })
            .run();
    });

const createImageThumbnail = (filePath: string, outputPath: string) =>
    sharp(filePath)
        .rotate()
        .resize(640, 640, { fit: 'inside', withoutEnlargement: true })
        .jpeg({ quality: 80, mozjpeg: true })
        .toFile(outputPath)
        .catch(() => null);

export const createThumbnail = async (file: { id: string; mimeType: string }, filePath: string) => {
    const thumbnailPath = getThumbnailPath(file.id);
    const tempPath = `${thumbnailPath}.tmp`;

    if (file.mimeType.startsWith('video/')) await createVideoThumbnail(filePath, tempPath);
    else await createImageThumbnail(filePath, tempPath);

    await fsp.rename(tempPath, thumbnailPath).catch(() => null);
};

export const processMedia = async (file: { id: string; fileName: string; mimeType: string }) => {
    if (canHaveThumbnail(file.mimeType) && !hasThumbnail(file.id))
        await createThumbnail(file, join(dataDirectory, 'uploads', file.fileName));
};
