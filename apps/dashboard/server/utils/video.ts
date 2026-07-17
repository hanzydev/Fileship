import fluentFfmpeg from 'fluent-ffmpeg';

import ffmpeg from '@ffmpeg-installer/ffmpeg';

fluentFfmpeg.setFfmpegPath(ffmpeg.path);

export const createVideoThumbnail = async (filePath: string, thumbnailPath: string) => {
    await new Promise<void>((resolve) => {
        const command = fluentFfmpeg(filePath)
            .inputOptions('-threads 1')
            .seekInput(1)
            .videoFilters('scale=640:-2')
            .frames(1)
            .format('mjpeg')
            .outputOptions('-threads 1')
            .output(thumbnailPath);

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
};
