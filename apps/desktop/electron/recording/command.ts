import type { AudioSourceId, AudioTrack, RecordingKind } from '../../shared/ipc';
import type { Settings, VideoCodec } from '../../shared/settings';
import { t } from '../i18n';
import type { Rect } from '../native';

export const SAMPLE_RATE = 48_000;

export const BYTES_PER_SECOND = SAMPLE_RATE * 2 * 4;

export const PRIMING_SECONDS = 0.1;

export const FIRST_FRAME_MARKER = 'Parsed_showinfo';

const SOURCE_TITLES: Record<AudioSourceId, string> = {
    microphone: 'recording.microphone',
    system: 'recording.systemAudio',
};

export const splitArgs = (value: string) =>
    [...value.matchAll(/"([^"]*)"|'([^']*)'|(\S+)/g)].map(
        (match) => match[1] ?? match[2] ?? match[3]!,
    );

export const audioTracks = (audio: Settings['audio']): AudioTrack[] => {
    const sources = (['microphone', 'system'] as const).filter((source) => audio[source]);
    if (!sources.length) return [];

    const separate = sources.map((source) => ({
        id: source,
        title: t(SOURCE_TITLES[source]),
        sources: [source],
    }));

    if (sources.length === 1 || audio.tracks === 'separate') return separate;

    const mixed: AudioTrack = { id: 'mixed', title: t('recording.mixedAudio'), sources };
    return audio.tracks === 'both' ? [mixed, ...separate] : [mixed];
};

const DEFAULT_PRESETS: Partial<Record<VideoCodec, string>> = {
    libx264: 'veryfast',
    libx265: 'veryfast',
    h264_nvenc: 'p4',
    hevc_nvenc: 'p4',
    av1_nvenc: 'p4',
    h264_amf: 'balanced',
    hevc_amf: 'balanced',
    av1_amf: 'balanced',
    h264_qsv: 'veryfast',
    hevc_qsv: 'veryfast',
    av1_qsv: 'veryfast',
    'libvpx-vp9': '8',
    libsvtav1: '10',
};

export const defaultPreset = (codec: VideoCodec) => DEFAULT_PRESETS[codec] ?? '';

export const pixelFormat = (codec: string) => (codec.endsWith('_qsv') ? 'nv12' : 'yuv420p');

export const videoCodecArgs = (
    codec: Exclude<VideoCodec, 'auto'>,
    recording: Settings['recording'],
) => {
    const preset = recording.preset || defaultPreset(codec);
    const quality = String(recording.quality);
    const kbps = recording.bitrate;
    const bitrate =
        recording.rateControl === 'bitrate'
            ? ['-b:v', `${kbps}k`, '-maxrate', `${kbps}k`, '-bufsize', `${kbps * 2}k`]
            : null;

    if (codec === 'libx264' || codec === 'libx265') {
        return ['-c:v', codec, '-preset', preset, ...(bitrate ?? ['-crf', quality])];
    }

    if (codec.endsWith('_nvenc')) {
        return [
            '-c:v',
            codec,
            '-preset',
            preset,
            '-tune',
            'hq',
            '-rc',
            'vbr',
            ...(bitrate ?? ['-cq', quality, '-b:v', '0']),
        ];
    }

    if (codec.endsWith('_amf')) {
        return [
            '-c:v',
            codec,
            '-quality',
            preset,
            ...(bitrate
                ? ['-rc', 'vbr_peak', ...bitrate]
                : ['-rc', 'cqp', '-qp_i', quality, '-qp_p', quality]),
        ];
    }

    if (codec.endsWith('_qsv')) {
        return ['-c:v', codec, '-preset', preset, ...(bitrate ?? ['-global_quality', quality])];
    }

    if (codec === 'libvpx-vp9') {
        return [
            '-c:v',
            codec,
            '-deadline',
            'realtime',
            '-cpu-used',
            preset,
            '-row-mt',
            '1',
            ...(bitrate ?? ['-crf', quality, '-b:v', '0']),
        ];
    }

    return ['-c:v', codec, '-preset', preset, ...(bitrate ?? ['-crf', quality])];
};

export const containerProblem = (container: string, codec: string) => {
    if (container !== 'webm') return null;
    if (codec === 'libvpx-vp9' || codec.startsWith('av1') || codec === 'libsvtav1') return null;
    return t('recording.webmCodec');
};

export type RecordTarget = {
    rect: Rect;
    output: { adapter: number; index: number; bounds: Rect } | null;
};

export type RecordCommand = {
    kind: RecordingKind;
    codec: Exclude<VideoCodec, 'auto'>;
    useDda: boolean;
    target: RecordTarget;
    pipes: string[];
    tracks: AudioTrack[];
    output: string;
};

const even = (value: number) => Math.max(2, value - (value % 2));

export const recordArgs = (settings: Settings, command: RecordCommand) => {
    const { recording, audio, gif } = settings;
    const { rect, output } = command.target;
    const isGif = command.kind === 'gif';
    const fps = isGif ? gif.fps : recording.fps;
    const width = even(rect.width);
    const height = even(rect.height);
    const format = isGif ? 'yuv444p' : pixelFormat(command.codec);
    const cursor = recording.showCursor ? 1 : 0;

    const args = [
        '-hide_banner',
        '-loglevel',
        'repeat+level+info',
        '-nostats',
        '-y',
        ...splitArgs(recording.inputArgs),
    ];
    let video: string;

    if (command.useDda && output) {
        const options = [
            `output_idx=${output.index}`,
            `framerate=${fps}`,
            `draw_mouse=${cursor}`,
            `offset_x=${rect.x - output.bounds.x}`,
            `offset_y=${rect.y - output.bounds.y}`,
            `video_size=${width}x${height}`,
        ].join(':');

        args.push('-init_hw_device', `d3d11va=dda:${output.adapter}`, '-filter_hw_device', 'dda');
        video = `ddagrab=${options},showinfo=checksum=0,hwdownload,format=bgra,format=${format}[v]`;
    } else {
        args.push(
            '-f',
            'gdigrab',
            '-framerate',
            String(fps),
            '-draw_mouse',
            String(cursor),
            '-offset_x',
            String(rect.x),
            '-offset_y',
            String(rect.y),
            '-video_size',
            `${width}x${height}`,
            '-i',
            'desktop',
        );

        video = `[0:v]showinfo=checksum=0,fps=${fps},format=${format}[v]`;
    }

    const firstAudio = command.useDda && output ? 0 : 1;
    const tracks = isGif ? [] : command.tracks;

    for (const pipe of command.pipes.slice(0, tracks.length)) {
        args.push(
            '-thread_queue_size',
            '4096',
            '-probesize',
            '32',
            '-analyzeduration',
            '0',
            '-itsoffset',
            String(audio.delay / 1000 - PRIMING_SECONDS),
            '-f',
            'f32le',
            '-ar',
            String(SAMPLE_RATE),
            '-ac',
            '2',
            '-i',
            pipe,
        );
    }

    args.push('-filter_complex', video, '-map', '[v]');

    for (const [index, track] of tracks.entries()) {
        args.push(
            '-map',
            `${firstAudio + index}:a`,
            `-filter:a:${index}`,
            'atrim=start=0',
            `-metadata:s:a:${index}`,
            `title=${track.title}`,
        );
    }

    if (isGif) {
        args.push('-c:v', 'libx264', '-preset', 'ultrafast', '-qp', '0');
    } else {
        args.push(...videoCodecArgs(command.codec, recording));
    }

    if (tracks.length) {
        const codec = recording.container === 'webm' ? 'libopus' : audio.codec;
        args.push('-c:a', codec, '-b:a', `${audio.bitrate}k`, '-ar', String(SAMPLE_RATE));
    }

    args.push(...(isGif ? [] : splitArgs(recording.outputArgs)), '-f', 'matroska', command.output);

    return args;
};

export const segmentInput = (segments: string[], list: string) =>
    segments.length === 1 ? ['-i', segments[0]!] : ['-f', 'concat', '-safe', '0', '-i', list];

export const joinArgs = (
    input: string[],
    output: string,
    container: string,
    tracks: AudioTrack[],
) => {
    return [
        '-hide_banner',
        '-loglevel',
        'error',
        '-y',
        ...input,
        '-map',
        '0',
        '-c',
        'copy',
        ...tracks.flatMap((track, index) => [
            `-metadata:s:a:${index}`,
            `title=${track.title}`,
            `-metadata:s:a:${index}`,
            `handler_name=${track.title}`,
        ]),
        ...(container === 'mp4' ? ['-movflags', '+faststart'] : []),
        output,
    ];
};

export const gifArgs = (input: string[], output: string, gif: Settings['gif']) => {
    const scale = gif.maxWidth ? `,scale='min(iw,${gif.maxWidth})':-1:flags=lanczos` : '';
    const dither = gif.dither === 'bayer' ? 'bayer:bayer_scale=3' : gif.dither;

    return [
        '-hide_banner',
        '-loglevel',
        'error',
        '-y',
        ...input,
        '-filter_complex',
        `[0:v]fps=${gif.fps}${scale},split[a][b];[a]palettegen=stats_mode=${gif.statsMode}[p];[b][p]paletteuse=dither=${dither}${gif.statsMode === 'diff' ? ':diff_mode=rectangle' : ''}`,
        '-loop',
        gif.loop ? '0' : '-1',
        output,
    ];
};

export const thumbnailArgs = (input: string) => [
    '-hide_banner',
    '-loglevel',
    'error',
    '-i',
    input,
    '-frames:v',
    '1',
    '-vf',
    'scale=480:-2',
    '-f',
    'image2pipe',
    '-c:v',
    'mjpeg',
    '-q:v',
    '5',
    'pipe:1',
];

export const formatCommand = (path: string, args: string[]) =>
    [path, ...args]
        .map((arg) => (/[\s"'[\];,]/.test(arg) ? `"${arg.replaceAll('"', '\\"')}"` : arg))
        .join(' ');
