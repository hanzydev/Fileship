import { existsSync, promises as fsp } from 'node:fs';

import fluentFfmpeg from 'fluent-ffmpeg';
import type { H3Event } from 'h3';
import { join } from 'pathe';
import sharp, { type SharpInput } from 'sharp';

import ffmpeg from '@ffmpeg-installer/ffmpeg';
import ffprobe from '@ffprobe-installer/ffprobe';
import { createPdfClient } from '@fileship/pdf';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);
fluentFfmpeg.setFfprobePath(ffprobe.path);

const pdf = createPdfClient();

export const getThumbnailPath = (id: string) => join(dataDirectory, 'thumbnails', `${id}.jpeg`);

export const hasThumbnail = (id: string) => existsSync(getThumbnailPath(id));

export const isAnalyzable = (mimeType: string) =>
    isPdf(mimeType) || ['image/', 'video/', 'audio/'].some((type) => mimeType.startsWith(type));

export const canHaveThumbnail = (mimeType: string) =>
    isPdf(mimeType) || mimeType.startsWith('video/') || isProcessableImage(mimeType);

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
    openImage(filePath)
        .rotate()
        .resize(640, 640, { fit: 'inside', withoutEnlargement: true })
        .jpeg({ quality: 80, mozjpeg: true })
        .toFile(outputPath)
        .catch(() => null);

const createPdfThumbnail = async (filePath: string, outputPath: string) => {
    const image = (await pdf.preview(filePath, 640).catch(() => null))?.image;
    if (!image) return;

    await sharp(image.data, { raw: { width: image.width, height: image.height, channels: 4 } })
        .removeAlpha()
        .jpeg({ quality: 80, mozjpeg: true })
        .toFile(outputPath)
        .catch(() => null);
};

const fitImage = (input: SharpInput, size: number) =>
    openImage(input).rotate().resize(size, size, { fit: 'inside', withoutEnlargement: true });

export const aiImage = (input: SharpInput) => fitImage(input, 1024).removeAlpha().png().toBuffer();

export const aiOcrImage = (input: SharpInput) => fitImage(input, 1600).png().toBuffer();

const readVideoFrame = (filePath: string, seconds: number) =>
    new Promise<Buffer>((resolve, reject) => {
        const chunks: Buffer[] = [];
        const command = fluentFfmpeg(filePath)
            .seekInput(seconds)
            .inputOptions('-threads 1')
            .frames(1)
            .videoFilters('scale=448:-2')
            .outputOptions(['-f image2pipe', '-vcodec mjpeg', '-threads 1'])
            .on('error', reject);

        const timeout = setTimeout(() => {
            command.kill('SIGKILL');
            reject(new Error('Reading a video frame timed out'));
        }, 60_000);

        const stream = command.pipe();
        stream.on('data', (chunk: Buffer) => chunks.push(chunk));
        stream.on('end', () => {
            clearTimeout(timeout);
            resolve(Buffer.concat(chunks));
        });
    });

export const extractVideoFrames = async (
    filePath: string,
    duration: number | undefined,
    count: number,
) => {
    const frames: Buffer[] = [];

    for (let i = 0; i < count; i++) {
        const seconds = duration ? (duration * (i + 0.5)) / count : i * 10;
        const frame = await readVideoFrame(filePath, seconds).catch(() => null);

        if (frame?.length) frames.push(await aiImage(frame));
    }

    if (!frames.length) throw new Error('No frames could be read from the video');

    return frames;
};

export const createThumbnail = async (file: { id: string; mimeType: string }, filePath: string) => {
    const thumbnailPath = getThumbnailPath(file.id);
    const tempPath = `${thumbnailPath}.tmp`;

    if (isPdf(file.mimeType)) await createPdfThumbnail(filePath, tempPath);
    else if (file.mimeType.startsWith('video/')) await createVideoThumbnail(filePath, tempPath);
    else await createImageThumbnail(filePath, tempPath);

    await fsp.rename(tempPath, thumbnailPath).catch(() => null);
};

const readImageMetadata = async (filePath: string): Promise<IMediaMetadata> => {
    const metadata = await openImage(filePath).metadata();
    const isRotated = metadata.orientation! >= 5;
    const height = metadata.pageHeight ?? metadata.height;

    return {
        width: isRotated ? height : metadata.width,
        height: isRotated ? metadata.width : height,
        format: metadata.format.toUpperCase(),
        colorSpace: metadata.space,
        bitDepth: metadata.bitsPerSample,
        hasAlpha: metadata.hasAlpha,
        frames: metadata.pages! > 1 ? metadata.pages : undefined,
    };
};

const probe = (filePath: string) =>
    new Promise<fluentFfmpeg.FfprobeData>((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error('ffprobe timed out')), 30_000);

        fluentFfmpeg.ffprobe(filePath, (error, data) => {
            clearTimeout(timeout);

            if (error) reject(error);
            else resolve(data);
        });
    });

const readStreamMetadata = async (filePath: string): Promise<IMediaMetadata> => {
    const { format, streams } = await probe(filePath);

    const video = streams.find(
        (stream) => stream.codec_type === 'video' && !stream.disposition?.attached_pic,
    );
    const audio = streams.find((stream) => stream.codec_type === 'audio');

    const isRotated = Math.abs(+(video?.rotation ?? 0)) === 90;
    const [frames, seconds] = (video?.avg_frame_rate ?? '0/0').split('/').map(Number);

    return {
        width: isRotated ? video?.height : video?.width,
        height: isRotated ? video?.width : video?.height,
        duration: +format.duration! || undefined,
        bitrate: +format.bit_rate! || undefined,
        fps: seconds ? Math.round((frames! / seconds) * 100) / 100 : undefined,
        format: format.format_long_name,
        videoCodec: video?.codec_name,
        audioCodec: audio?.codec_name,
        sampleRate: Number(audio?.sample_rate) || undefined,
        channels: audio?.channels,
    };
};

const readPdfMetadata = async (filePath: string): Promise<IMediaMetadata> => {
    const { pages, version } = (await pdf.preview(filePath, 0))!;

    return {
        pages,
        format: version ? `PDF ${Math.floor(version / 10)}.${version % 10}` : 'PDF',
    };
};

export const readMetadata = (mimeType: string, filePath: string) =>
    (isPdf(mimeType)
        ? readPdfMetadata(filePath)
        : mimeType.startsWith('image/')
          ? readImageMetadata(filePath)
          : readStreamMetadata(filePath)
    ).catch(() => ({}) as IMediaMetadata);

export const processMedia = async (file: { id: string; mimeType: string }, filePath: string) => {
    if (canHaveThumbnail(file.mimeType) && !hasThumbnail(file.id))
        await createThumbnail(file, filePath);

    const metadata = await readMetadata(file.mimeType, filePath);

    if (canHaveThumbnail(file.mimeType) && !hasThumbnail(file.id)) metadata.thumbnailFailed = true;

    await prisma.file.update({ where: { id: file.id }, data: { metadata: metadata as any } });

    return metadata;
};
