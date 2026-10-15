import { execFile } from 'node:child_process';
import { createHash } from 'node:crypto';
import { createWriteStream, existsSync } from 'node:fs';
import { mkdir, rename, rm, stat } from 'node:fs/promises';
import { join } from 'node:path';
import { promisify } from 'node:util';

import { app, net } from 'electron';
import log from 'electron-log/main';

import type { FfmpegStatus } from '../../shared/ipc';
import { HARDWARE_CODECS, VIDEO_CODECS } from '../../shared/settings';
import { getSettings, getStore, updateStore } from '../store';
import { broadcast } from '../windows';

const run = promisify(execFile);

const BUILD = {
    version: '9.0.2',
    url: 'https://github.com/GyanD/codexffmpeg/releases/download/9.0.2/ffmpeg-9.0.2-essentials_build.zip',
    sha256: '60f467265b1e312373dbcd92200c2618a74850f98d3d078e94296bb3fa2047ba',
};

const managedDir = () => join(app.getPath('userData'), 'ffmpeg');
const managedPath = () => join(managedDir(), 'ffmpeg.exe');

let download: { progress: number; error: string | null } | null = null;
type Probe = { path: string; version: string; ddagrab: boolean; encoders: string[] };

let probed: Probe | null = null;
let probing: { path: string; promise: Promise<Probe> } | null = null;

export const ffmpegPath = () => {
    const custom = getSettings().recording.ffmpegPath;
    if (custom) return existsSync(custom) ? custom : null;
    return existsSync(managedPath()) ? managedPath() : null;
};

const probeKey = async (path: string) => {
    const info = await stat(path);
    return `${path}|${info.size}|${info.mtimeMs}`;
};

const runProbe = async (path: string): Promise<Probe> => {
    const key = await probeKey(path);
    const cached = getStore().ffmpegProbe;

    if (cached?.key === key) {
        const { version, ddagrab, encoders } = cached;
        return { path, version, ddagrab, encoders };
    }

    const { stdout: version } = await run(path, ['-hide_banner', '-version']);
    const { stdout: filters } = await run(path, ['-hide_banner', '-filters']);
    const { stdout: encoders } = await run(path, ['-hide_banner', '-encoders']);

    const available = [...encoders.matchAll(/^\s*V\S*\s+(\S+)/gm)].map((match) => match[1]!);
    const working: string[] = available.filter((name) => !name.includes('_'));

    const hardware = available.filter(
        (name) =>
            (VIDEO_CODECS as readonly string[]).includes(name) && /_(nvenc|amf|qsv)$/.test(name),
    );

    for (const encoder of hardware) {
        await run(path, [
            '-hide_banner',
            '-loglevel',
            'error',
            '-f',
            'lavfi',
            '-i',
            'color=c=black:s=256x256:r=30',
            '-frames:v',
            '3',
            '-c:v',
            encoder,
            '-f',
            'null',
            '-',
        ]).then(
            () => working.push(encoder),
            () => null,
        );
    }

    const result = {
        path,
        version: /ffmpeg version (\S+)/.exec(version)?.[1] ?? 'unknown',
        ddagrab: /\bddagrab\b/.test(filters),
        encoders: working,
    };

    const { path: _path, ...rest } = result;
    updateStore({ ffmpegProbe: { key, ...rest } });

    return result;
};

const probe = async (path: string) => {
    if (probed?.path === path) return probed;
    if (probing?.path === path) return probing.promise;

    const promise = runProbe(path);
    probing = { path, promise };

    try {
        probed = await promise;
        return probed;
    } finally {
        if (probing?.promise === promise) probing = null;
    }
};

export const getFfmpeg = async () => {
    const path = ffmpegPath();
    if (!path) return null;

    try {
        return await probe(path);
    } catch (error) {
        log.warn('Could not run ffmpeg', error);
        return null;
    }
};

export const autoCodec = (encoders: string[]) =>
    HARDWARE_CODECS.find((codec) => encoders.includes(codec)) ?? 'libx264';

export const getFfmpegStatus = async (): Promise<FfmpegStatus> => {
    const info = await getFfmpeg();
    const custom = getSettings().recording.ffmpegPath;

    return {
        path: info?.path ?? (custom || null),
        version: info?.version ?? null,
        isManaged: !custom,
        isInstalled: !!info,
        ddagrab: info?.ddagrab ?? false,
        encoders: info?.encoders ?? [],
        managedVersion: BUILD.version,
        download,
    };
};

const publish = async () => broadcast('ffmpeg:status', await getFfmpegStatus());

export const resetFfmpegProbe = () => {
    probed = null;
    publish();
};

export const downloadFfmpeg = async () => {
    if (download && !download.error) return;

    download = { progress: 0, error: null };
    publish();

    const folder = managedDir();
    const archive = join(app.getPath('temp'), `fileship-ffmpeg-${Date.now()}.zip`);
    const extracted = join(app.getPath('temp'), `fileship-ffmpeg-${Date.now()}`);

    try {
        const response = await net.fetch(BUILD.url);
        if (!response.ok || !response.body) throw new Error(`Download failed (${response.status})`);

        const total = Number(response.headers.get('content-length')) || 0;
        const hash = createHash('sha256');
        const file = createWriteStream(archive);
        const reader = response.body.getReader();
        let received = 0;
        let lastPublish = 0;

        for (;;) {
            const { done, value } = await reader.read();
            if (done) break;

            hash.update(value);
            received += value.length;

            if (!file.write(value)) await new Promise((resolve) => file.once('drain', resolve));

            if (total && Date.now() - lastPublish > 250) {
                lastPublish = Date.now();
                download.progress = Math.round((received / total) * 100);
                publish();
            }
        }

        await new Promise<void>((resolve, reject) =>
            file.end((error?: Error | null) => (error ? reject(error) : resolve())),
        );

        if (hash.digest('hex') !== BUILD.sha256) {
            throw new Error('The download is damaged. Try again.');
        }

        await mkdir(extracted, { recursive: true });
        await run(join(process.env.SystemRoot ?? 'C:\\Windows', 'System32', 'tar.exe'), [
            '-xf',
            archive,
            '-C',
            extracted,
            '--strip-components',
            '2',
            '*/bin/ffmpeg.exe',
        ]);

        const binary = join(extracted, 'ffmpeg.exe');
        if (!(await stat(binary)).isFile())
            throw new Error('ffmpeg.exe is missing in the download');

        await mkdir(folder, { recursive: true });
        await rm(managedPath(), { force: true });
        await rename(binary, managedPath());

        download = null;
        probed = null;
    } catch (error: any) {
        log.error('Could not download ffmpeg', error);
        download = { progress: 0, error: error.message };
    } finally {
        await rm(archive, { force: true }).catch(() => {});
        await rm(extracted, { recursive: true, force: true }).catch(() => {});
        publish();
    }
};
