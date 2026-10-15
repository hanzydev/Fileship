import { type ChildProcess, execFile, spawn } from 'node:child_process';
import { mkdir, rm, stat, writeFile } from 'node:fs/promises';
import { createServer, type Server, type Socket } from 'node:net';
import { join } from 'node:path';
import { promisify } from 'node:util';

import { app, BrowserWindow, desktopCapturer, Notification, screen, session } from 'electron';
import log from 'electron-log/main';

import type {
    AudioSourceId,
    AudioTrack,
    RecorderControl,
    RecorderState,
    RecordingKind,
} from '../../shared/ipc';
import type { Settings, VideoCodec } from '../../shared/settings';
import { physicalBounds } from '../capture';
import { listOutputs, type Rect } from '../capture/native';
import { openRegionCapture } from '../overlay';
import { getSettings } from '../store';
import { formatFileName, handleRecording, saveFolder } from '../tasks';
import { refreshTray } from '../tray';
import { notifyIdle } from '../updater';
import { broadcast, iconPath, isAppUrl, rendererUrl, showMainWindow } from '../windows';
import {
    audioTracks,
    BYTES_PER_SECOND,
    containerProblem,
    FIRST_FRAME_MARKER,
    formatCommand,
    gifArgs,
    joinArgs,
    PRIMING_SECONDS,
    recordArgs,
    SAMPLE_RATE,
    segmentInput,
    thumbnailArgs,
} from './command';
import { autoCodec, getFfmpeg } from './ffmpeg';

const run = promisify(execFile);

const CAPTURE_LATENCY_MS = 100;

const MIME_TYPES: Record<string, string> = {
    mp4: 'video/mp4',
    mkv: 'video/x-matroska',
    webm: 'video/webm',
    gif: 'image/gif',
};

type PendingChunk = { at: number; data: Buffer };

type Segment = {
    output: string;
    videoStartedAt: number | null;
    pending: PendingChunk[][];
    aligned: boolean[];
    process: ChildProcess;
    servers: Server[];
    sockets: (Socket | null)[];
    exited: Promise<number | null>;
    stderr: () => string;
    isStopping: boolean;
};

type Recording = {
    kind: RecordingKind;
    settings: Settings;
    ffmpeg: string;
    codec: Exclude<VideoCodec, 'auto'>;
    tracks: AudioTrack[];
    useDda: boolean;
    target: { rect: Rect; output: { adapter: number; index: number; bounds: Rect } | null };
    folder: string;
    segments: string[];
    segment: Segment | null;
    toolbar: BrowserWindow | null;
    border: BrowserWindow | null;
    markReady: ((error: string | null) => void) | null;
    timer: NodeJS.Timeout | null;
};

const idleState = (kind: RecordingKind = 'video'): RecorderState => ({
    phase: 'idle',
    kind,
    elapsed: 0,
    since: null,
    countdown: 0,
    tracks: [],
    muted: [],
    maxDuration: 0,
});

let state = idleState();
let current: Recording | null = null;

const notify = (title: string, body: string) => {
    if (Notification.isSupported()) new Notification({ title, body, icon: iconPath() }).show();
};

const setState = (patch: Partial<RecorderState>) => {
    state = { ...state, ...patch };
    broadcast('recorder:state', state);
    refreshTray();
};

export const getRecorderState = () => state;

export const isRecording = () => state.phase !== 'idle';

const contains = (outer: Rect, inner: Rect) =>
    inner.x >= outer.x &&
    inner.y >= outer.y &&
    inner.x + inner.width <= outer.x + outer.width &&
    inner.y + inner.height <= outer.y + outer.height;

const closeWindows = (recording: Recording) => {
    const windows = [recording.toolbar, recording.border];

    recording.toolbar = null;
    recording.border = null;

    for (const window of windows) {
        if (window && !window.isDestroyed()) window.destroy();
    }
};

const createWindows = (recording: Recording) => {
    const dip = screen.screenToDipRect(null, recording.target.rect);
    const { workArea } = screen.getDisplayMatching(dip);
    const preferences = {
        preload: join(__dirname, 'preload.cjs'),
        contextIsolation: true,
        sandbox: true,
        nodeIntegration: false,
        backgroundThrottling: false,
    };

    const shared = {
        frame: false,
        transparent: true,
        resizable: false,
        movable: false,
        minimizable: false,
        maximizable: false,
        fullscreenable: false,
        skipTaskbar: true,
        alwaysOnTop: true,
        hasShadow: false,
        show: false,
        webPreferences: preferences,
    };

    if (recording.settings.recording.showBorder) {
        const border = new BrowserWindow({
            ...shared,
            x: dip.x - 3,
            y: dip.y - 3,
            width: dip.width + 6,
            height: dip.height + 6,
            focusable: false,
        });

        border.setIgnoreMouseEvents(true);
        border.setContentProtection(true);
        border.setAlwaysOnTop(true, 'screen-saver');
        border.loadURL(rendererUrl('/recorder?view=border'));
        border.once('ready-to-show', () => border.showInactive());
        recording.border = border;
    }

    const width = 440;
    const height = 64;
    const x = Math.round(
        Math.min(
            Math.max(dip.x + dip.width / 2 - width / 2, workArea.x),
            workArea.x + workArea.width - width,
        ),
    );

    let y = dip.y + dip.height + 8;
    if (y + height > workArea.y + workArea.height) y = dip.y - height - 8;
    if (y < workArea.y) y = dip.y + dip.height - height - 8;

    const toolbar = new BrowserWindow({ ...shared, x, y: Math.round(y), width, height });

    toolbar.setContentProtection(true);
    toolbar.setAlwaysOnTop(true, 'screen-saver');
    toolbar.loadURL(rendererUrl('/recorder'));
    toolbar.once('ready-to-show', () => toolbar.showInactive());
    toolbar.on('closed', () => {
        if (current === recording && recording.toolbar === toolbar) abortRecording();
    });

    recording.toolbar = toolbar;
};

const listen = (pipe: string, onSocket: (socket: Socket) => void) =>
    new Promise<Server>((resolve, reject) => {
        const server = createServer((socket) => {
            socket.on('error', () => socket.destroy());
            onSocket(socket);
        });

        server.once('error', reject);
        server.listen(pipe, () => resolve(server));
    });

const startSegment = async (recording: Recording) => {
    const index = recording.segments.length;
    const output = join(recording.folder, `segment-${index}.mkv`);
    const { tracks } = recording;
    const pipes = tracks.map(
        (_, track) => `\\\\.\\pipe\\fileship-${process.pid}-${Date.now()}-${index}-${track}`,
    );
    const sockets: (Socket | null)[] = pipes.map(() => null);
    const servers = await Promise.all(
        pipes.map((pipe, track) =>
            listen(pipe, (socket) => {
                sockets[track] = socket;
                socket.write(Buffer.alloc(Math.round(BYTES_PER_SECOND * PRIMING_SECONDS)));
            }),
        ),
    );

    const args = recordArgs(recording.settings, {
        kind: recording.kind,
        codec: recording.codec,
        useDda: recording.useDda,
        target: recording.target,
        pipes,
        tracks,
        output,
    });

    log.info('Recording', recording.ffmpeg, args.join(' '));

    const child = spawn(recording.ffmpeg, args, {
        stdio: ['pipe', 'ignore', 'pipe'],
        windowsHide: true,
    });

    let stderr = '';
    let partial = '';

    child.stderr!.on('data', (data) => {
        const lines = (partial + data).split(/\r?\n/);
        partial = lines.pop() ?? '';

        for (const line of lines) {
            if (!segment.videoStartedAt && line.includes(FIRST_FRAME_MARKER)) {
                markVideoStarted(segment);
            }
            if (/^\[(error|fatal|panic)\]/.test(line)) {
                stderr = `${stderr}${line.replace(/^\[\w+\]\s*/, '')}\n`.slice(-4000);
            }
        }
    });
    child.stdin!.on('error', () => {});

    const exited = new Promise<number | null>((resolve) => {
        child.once('exit', (code) => resolve(code));
        child.once('error', (error) => {
            stderr += error.message;
            resolve(-1);
        });
    });

    const segment: Segment = {
        output,
        videoStartedAt: null,
        pending: tracks.map(() => []),
        aligned: tracks.map(() => false),
        process: child,
        servers,
        sockets,
        exited,
        stderr: () => stderr,
        isStopping: false,
    };

    recording.segment = segment;
    recording.segments.push(output);
    setState({ phase: 'recording', since: Date.now() });

    exited.then((code) => {
        if (segment.isStopping || current !== recording) return;

        log.error('ffmpeg stopped while recording', code, stderr);
        notify('Recording stopped', lastLine(stderr) || `ffmpeg exited with code ${code}`);
        stopRecording();
    });
};

const lastLine = (text: string) =>
    text.trim().split(/\r?\n/).filter(Boolean).at(-1)?.slice(0, 200) ?? '';

const closeSegment = async (recording: Recording, segment: Segment, isAbort = false) => {
    segment.isStopping = true;

    if (state.since) setState({ elapsed: state.elapsed + Date.now() - state.since, since: null });

    if (isAbort) {
        segment.process.kill();
    } else {
        segment.process.stdin!.write('q');
        for (const socket of segment.sockets) socket?.end();
    }

    const timeout = new Promise<'timeout'>((resolve) =>
        setTimeout(() => resolve('timeout'), 15_000),
    );
    if ((await Promise.race([segment.exited, timeout])) === 'timeout') {
        log.warn('ffmpeg did not stop in time', segment.stderr());
        segment.process.kill();
    }

    const code = await segment.exited;
    if (code) log.warn('ffmpeg exited with', code, segment.stderr());

    for (const socket of segment.sockets) socket?.destroy();
    for (const server of segment.servers) server.close();

    if (recording.segment === segment) recording.segment = null;
};

const finish = async (recording: Recording) => {
    if (recording.timer) clearInterval(recording.timer);
    closeWindows(recording);

    if (current === recording) current = null;
    setState(idleState(recording.kind));
    notifyIdle();
};

const runFfmpeg = async (path: string, args: string[]) => {
    try {
        await run(path, args, { windowsHide: true, maxBuffer: 1024 * 1024 });
    } catch (error: any) {
        throw new Error(lastLine(String(error.stderr ?? '')) || error.message, { cause: error });
    }
};

const produce = async (recording: Recording) => {
    const { kind, settings, ffmpeg } = recording;
    const segments: string[] = [];

    for (const segment of recording.segments) {
        const size = await stat(segment).then(
            (info) => info.size,
            () => 0,
        );

        if (size > 1024) segments.push(segment);
    }

    if (!segments.length) throw new Error('Nothing was recorded.');

    const list = join(recording.folder, 'segments.txt');
    await writeFile(
        list,
        segments
            .map((path) => `file '${path.replaceAll('\\', '/').replaceAll("'", "'\\''")}'`)
            .join('\n'),
    );

    const extension = kind === 'gif' ? 'gif' : settings.recording.container;
    const isSaved = settings.afterCapture.saveToFile || !settings.afterCapture.upload;
    const folder = isSaved ? saveFolder() : recording.folder;
    const name = `${formatFileName(settings.save.pattern) || 'recording'}.${extension}`;
    const output = join(folder, name);
    const input = segmentInput(segments, list);

    await mkdir(folder, { recursive: true });
    await runFfmpeg(
        ffmpeg,
        kind === 'gif'
            ? gifArgs(input, output, settings.gif)
            : joinArgs(input, output, settings.recording.container, recording.tracks),
    );

    const thumbnail = await run(ffmpeg, thumbnailArgs(output), {
        encoding: 'buffer',
        windowsHide: true,
        maxBuffer: 16 * 1024 * 1024,
    }).then(
        ({ stdout }) => `data:image/jpeg;base64,${stdout.toString('base64')}`,
        () => null,
    );

    return { output, name, isSaved, mimeType: MIME_TYPES[extension]!, thumbnail };
};

export const stopRecording = async () => {
    const recording = current;
    if (!recording || state.phase === 'processing') return;

    if (state.phase === 'preparing' || state.phase === 'countdown') return abortRecording();

    if (recording.segment) await closeSegment(recording, recording.segment);
    setState({ phase: 'processing' });

    let result: Awaited<ReturnType<typeof produce>> | null = null;

    try {
        result = await produce(recording);
    } catch (error: any) {
        log.error('Could not finish the recording', error);
        notify('Could not finish the recording', error.message);
    }

    await finish(recording);

    if (result) {
        await handleRecording({
            kind: recording.kind === 'gif' ? 'gif' : 'recording',
            path: result.output,
            name: result.name,
            mimeType: result.mimeType,
            thumbnail: result.thumbnail,
            isSaved: result.isSaved,
        });
    }

    await rm(recording.folder, { recursive: true, force: true }).catch(() => {});
};

export const abortRecording = async () => {
    const recording = current;
    if (!recording) return;

    current = null;
    recording.markReady?.('aborted');
    if (recording.segment) await closeSegment(recording, recording.segment, true);

    await finish(recording);
    await rm(recording.folder, { recursive: true, force: true }).catch(() => {});
};

export const pauseRecording = async () => {
    const recording = current;
    if (!recording?.segment || state.phase !== 'recording') return;

    await closeSegment(recording, recording.segment);
    if (current === recording) setState({ phase: 'paused' });
};

export const resumeRecording = async () => {
    const recording = current;
    if (!recording || state.phase !== 'paused') return;

    try {
        await startSegment(recording);
    } catch (error: any) {
        notify('Could not resume the recording', error.message);
        stopRecording();
    }
};

export const controlRecording = (action: RecorderControl) => {
    if (action === 'stop') return stopRecording();
    if (action === 'abort') return abortRecording();
    if (action === 'pause') return pauseRecording();
    return resumeRecording();
};

export const setRecordingMute = (source: AudioSourceId, muted: boolean) => {
    const others = state.muted.filter((item) => item !== source);
    setState({ muted: muted ? [...others, source] : others });
};

export const markRecorderReady = (error: string | null) => current?.markReady?.(error);

const writeAudio = (segment: Segment, track: number, data: Buffer) => {
    const socket = segment.sockets[track];
    if (socket && !socket.destroyed) socket.write(data);
};

const writeAligned = (segment: Segment, track: number, at: number, data: Buffer) => {
    if (segment.aligned[track]) return writeAudio(segment, track, data);

    const startedAt = segment.videoStartedAt!;
    const endedAt = at - CAPTURE_LATENCY_MS;
    const beganAt = endedAt - (data.length / BYTES_PER_SECOND) * 1000;
    if (endedAt <= startedAt) return;

    segment.aligned[track] = true;

    const skip = Math.max(0, Math.round(((startedAt - beganAt) / 1000) * SAMPLE_RATE)) * 8;
    writeAudio(segment, track, data.subarray(Math.min(skip, data.length)));
};

const markVideoStarted = (segment: Segment) => {
    if (segment.videoStartedAt) return;

    segment.videoStartedAt = Date.now();

    for (const [track, chunks] of segment.pending.entries()) {
        for (const { at, data } of chunks) writeAligned(segment, track, at, data);
        chunks.length = 0;
    }
};

export const writeRecordingAudio = (track: number, chunk: ArrayBuffer) => {
    const segment = current?.segment;
    if (state.phase !== 'recording' || !segment || segment.isStopping) return;

    const data = Buffer.from(chunk);
    if (segment.videoStartedAt) return writeAligned(segment, track, Date.now(), data);

    const pending = segment.pending[track];
    if (!pending) return;

    pending.push({ at: Date.now(), data });
    if (pending.length > 100) pending.shift();
};

const findOutput = (rect: Rect) => {
    try {
        const match = listOutputs().find((item) => contains(item.rect, rect));
        return match ? { adapter: match.adapter, index: match.output, bounds: match.rect } : null;
    } catch (error) {
        log.warn('Could not list the displays', error);
        return null;
    }
};

const begin = async (kind: RecordingKind, rect: Rect) => {
    const ffmpeg = await getFfmpeg();
    if (!ffmpeg) return;

    const settings = getSettings();
    const codec =
        kind === 'gif'
            ? 'libx264'
            : settings.recording.codec === 'auto'
              ? autoCodec(ffmpeg.encoders)
              : settings.recording.codec;

    if (!ffmpeg.encoders.includes(codec)) {
        notify(
            'Encoder not available',
            `${codec} does not work on this computer. Pick another one.`,
        );
        showMainWindow('/settings/recording');
        return;
    }

    const problem = kind === 'video' ? containerProblem(settings.recording.container, codec) : null;

    if (problem) {
        notify('Cannot record', problem);
        showMainWindow('/settings/recording');
        return;
    }

    const output = findOutput(rect);

    const recording: Recording = {
        kind,
        settings,
        ffmpeg: ffmpeg.path,
        codec,
        tracks: kind === 'gif' ? [] : audioTracks(settings.audio),
        useDda: settings.recording.source !== 'gdigrab' && ffmpeg.ddagrab && !!output,
        target: { rect, output },
        folder: join(app.getPath('temp'), `fileship-recording-${Date.now()}`),
        segments: [],
        segment: null,
        toolbar: null,
        border: null,
        markReady: null,
        timer: null,
    };

    current = recording;
    await mkdir(recording.folder, { recursive: true });

    setState({
        ...idleState(kind),
        phase: 'preparing',
        tracks: recording.tracks,
        maxDuration: settings.recording.maxDuration * 1000,
    });

    const ready = new Promise<string | null>((resolve) => {
        recording.markReady = (error) => {
            recording.markReady = null;
            resolve(error);
        };

        setTimeout(() => recording.markReady?.('The recorder did not start in time.'), 15_000);
    });

    createWindows(recording);

    const error = await ready;
    if (current !== recording) return;

    if (error) {
        notify('Could not start recording', error);
        return abortRecording();
    }

    for (let seconds = settings.recording.startDelay; seconds > 0; seconds--) {
        setState({ phase: 'countdown', countdown: seconds });
        await new Promise((resolve) => setTimeout(resolve, 1000));
        if (current !== recording) return;
    }

    try {
        await startSegment(recording);
    } catch (startError: any) {
        log.error('Could not start ffmpeg', startError);
        notify('Could not start recording', startError.message);
        return abortRecording();
    }

    recording.timer = setInterval(() => {
        const elapsed = state.elapsed + (state.since ? Date.now() - state.since : 0);
        if (state.maxDuration && elapsed >= state.maxDuration) stopRecording();
    }, 250);
};

export const toggleRecording = async (kind: RecordingKind) => {
    if (state.phase === 'recording' || state.phase === 'paused') return stopRecording();
    if (state.phase === 'preparing' || state.phase === 'countdown') return abortRecording();
    if (state.phase !== 'idle') return;

    if (!(await getFfmpeg())) {
        notify('Recording needs ffmpeg', 'Download it in the recording settings, then try again.');
        showMainWindow('/settings/recording');
        return;
    }

    openRegionCapture(kind, (rect) => {
        begin(kind, rect).catch((error) => {
            log.error('Could not start recording', error);
            notify('Could not start recording', error.message);
            abortRecording();
        });
    });
};

export const quitRecorder = () => {
    current?.segment?.process.stdin?.write('q');
};

export const registerRecorder = () => {
    session.defaultSession.setDisplayMediaRequestHandler((request, callback) => {
        if (!isAppUrl(request.frame?.url ?? '')) return callback({});

        desktopCapturer
            .getSources({ types: ['screen'], thumbnailSize: { width: 0, height: 0 } })
            .then(([source]) => callback(source ? { video: source, audio: 'loopback' } : {}))
            .catch(() => callback({}));
    });
};

export const recordingPreview = async () => {
    const ffmpeg = await getFfmpeg();
    const settings = getSettings();
    const codec =
        settings.recording.codec === 'auto'
            ? autoCodec(ffmpeg?.encoders ?? [])
            : settings.recording.codec;
    const bounds = physicalBounds(screen.getPrimaryDisplay());
    const tracks = audioTracks(settings.audio);

    const args = recordArgs(settings, {
        kind: 'video',
        codec,
        useDda: settings.recording.source !== 'gdigrab' && (ffmpeg?.ddagrab ?? true),
        target: { rect: bounds, output: findOutput(bounds) ?? { adapter: 0, index: 0, bounds } },
        pipes: tracks.map((track) => `\\\\.\\pipe\\fileship-${track.id}`),
        tracks,
        output: 'recording.mkv',
    });

    return formatCommand(ffmpeg?.path ?? 'ffmpeg.exe', args);
};
