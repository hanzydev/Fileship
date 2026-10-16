import { z } from 'zod';

export const HOTKEY_ACTIONS = [
    'captureMonitor',
    'captureRegion',
    'captureWindow',
    'captureText',
    'recordVideo',
    'recordGif',
    'captureFullscreen',
    'captureLastRegion',
    'uploadClipboard',
    'uploadFile',
    'shortenClipboard',
    'openMain',
] as const;

export type HotkeyAction = (typeof HOTKEY_ACTIONS)[number];

export const DEFAULT_HOTKEYS: Record<HotkeyAction, string> = {
    captureMonitor: 'PrintScreen',
    captureRegion: 'Control+PrintScreen',
    captureWindow: 'Alt+PrintScreen',
    captureText: 'Control+Alt+PrintScreen',
    recordVideo: 'Shift+PrintScreen',
    recordGif: 'Control+Shift+PrintScreen',
    captureFullscreen: '',
    captureLastRegion: '',
    uploadClipboard: '',
    uploadFile: '',
    shortenClipboard: '',
    openMain: '',
};

export const FILE_NAME_TYPES = ['Random', 'UUID', 'Original'] as const;

export const EXPIRATIONS = [
    { label: 'labels.expiration.never', value: 0 },
    { label: 'labels.expiration.5m', value: 300_000 },
    { label: 'labels.expiration.15m', value: 900_000 },
    { label: 'labels.expiration.30m', value: 1_800_000 },
    { label: 'labels.expiration.1h', value: 3_600_000 },
    { label: 'labels.expiration.6h', value: 21_600_000 },
    { label: 'labels.expiration.12h', value: 43_200_000 },
    { label: 'labels.expiration.1d', value: 86_400_000 },
    { label: 'labels.expiration.1w', value: 604_800_000 },
    { label: 'labels.expiration.2w', value: 1_209_600_000 },
    { label: 'labels.expiration.1mo', value: 2_592_000_000 },
    { label: 'labels.expiration.3mo', value: 7_776_000_000 },
    { label: 'labels.expiration.6mo', value: 15_552_000_000 },
    { label: 'labels.expiration.1y', value: 31_536_000_000 },
];

export const COMPRESSIONS = [
    { label: 'labels.compression.none', value: 0 },
    { label: 'labels.compression.low', value: 25 },
    { label: 'labels.compression.medium', value: 50 },
    { label: 'labels.compression.high', value: 75 },
    { label: 'labels.compression.maximum', value: 100 },
];

export const VIDEO_SOURCES = ['auto', 'ddagrab', 'gdigrab'] as const;

export const VIDEO_CODECS = [
    'auto',
    'libx264',
    'libx265',
    'h264_nvenc',
    'hevc_nvenc',
    'av1_nvenc',
    'h264_amf',
    'hevc_amf',
    'av1_amf',
    'h264_qsv',
    'hevc_qsv',
    'av1_qsv',
    'libvpx-vp9',
    'libsvtav1',
] as const;

export const HARDWARE_CODECS = ['h264_nvenc', 'h264_amf', 'h264_qsv'] as const;

export const CONTAINERS = ['mp4', 'mkv', 'webm'] as const;

export const AUDIO_CODECS = ['aac', 'libopus', 'libmp3lame'] as const;

export const AUDIO_TRACKS = ['mixed', 'separate', 'both'] as const;

export const GIF_DITHERS = ['sierra2_4a', 'floyd_steinberg', 'bayer', 'none'] as const;

export type VideoCodec = (typeof VIDEO_CODECS)[number];

export const settingsSchema = z.object({
    startAtLogin: z.boolean().default(true),
    theme: z.enum(['system', 'light', 'dark']).default('system'),
    language: z.enum(['system', 'en', 'tr']).default('system'),
    hotkeys: z
        .partialRecord(z.enum(HOTKEY_ACTIONS), z.string().max(64))
        .default({})
        .transform((hotkeys) => ({ ...DEFAULT_HOTKEYS, ...hotkeys })),
    hotkeysEnabled: z.boolean().default(true),
    capture: z
        .object({
            showCursor: z.boolean().default(true),
            detectWindows: z.boolean().default(true),
            detectControls: z.boolean().default(true),
            showMagnifier: z.boolean().default(true),
            playSound: z.boolean().default(true),
        })
        .prefault({}),
    afterCapture: z
        .object({
            copyImage: z.boolean().default(false),
            saveToFile: z.boolean().default(false),
            annotate: z.boolean().default(false),
            upload: z.boolean().default(true),
        })
        .prefault({}),
    afterUpload: z
        .object({
            copyUrl: z.boolean().default(true),
            shortenUrl: z.boolean().default(false),
            openUrl: z.boolean().default(false),
            notify: z.boolean().default(true),
            playSound: z.boolean().default(true),
        })
        .prefault({}),
    upload: z
        .object({
            fileNameType: z.enum(FILE_NAME_TYPES).default('Random'),
            expiration: z.number().int().min(0).default(0),
            maxViews: z.number().int().min(0).default(0),
            compression: z.number().int().min(0).max(100).default(0),
            password: z.string().max(48).default(''),
            folderId: z.string().nullable().default(null),
        })
        .prefault({}),
    drive: z
        .object({
            enabled: z.boolean().default(true),
            folder: z.string().max(1024).default(''),
        })
        .prefault({}),
    save: z
        .object({
            folder: z.string().default(''),
            pattern: z.string().min(1).max(128).default('%yyyy-%mo-%d_%h-%mi-%s'),
        })
        .prefault({}),
    recording: z
        .object({
            ffmpegPath: z.string().max(1024).default(''),
            source: z.enum(VIDEO_SOURCES).default('auto').catch('auto'),
            fps: z.number().int().min(1).max(240).default(30),
            showCursor: z.boolean().default(true),
            codec: z.enum(VIDEO_CODECS).default('auto'),
            rateControl: z.enum(['quality', 'bitrate']).default('quality'),
            quality: z.number().int().min(0).max(63).default(23),
            bitrate: z.number().int().min(100).max(500_000).default(8000),
            preset: z.string().max(32).default(''),
            container: z.enum(CONTAINERS).default('mp4'),
            inputArgs: z.string().max(2000).default(''),
            outputArgs: z.string().max(2000).default(''),
            startDelay: z.number().int().min(0).max(30).default(0),
            maxDuration: z.number().int().min(0).max(86_400).default(0),
            showBorder: z.boolean().default(true),
        })
        .prefault({}),
    audio: z
        .object({
            microphone: z.boolean().default(false),
            microphoneId: z.string().max(512).default(''),
            microphoneVolume: z.number().int().min(0).max(300).default(100),
            noiseSuppression: z.boolean().default(true),
            echoCancellation: z.boolean().default(false),
            autoGainControl: z.boolean().default(false),
            system: z.boolean().default(false),
            systemVolume: z.number().int().min(0).max(300).default(100),
            tracks: z.enum(AUDIO_TRACKS).default('mixed'),
            codec: z.enum(AUDIO_CODECS).default('aac'),
            bitrate: z.number().int().min(32).max(512).default(192),
            delay: z.number().int().min(-2000).max(2000).default(0),
        })
        .prefault({}),
    gif: z
        .object({
            fps: z.number().int().min(1).max(50).default(15),
            maxWidth: z.number().int().min(0).max(7680).default(0),
            dither: z.enum(GIF_DITHERS).default('sierra2_4a'),
            statsMode: z.enum(['diff', 'full']).default('diff'),
            loop: z.boolean().default(true),
        })
        .prefault({}),
});

export type Settings = z.output<typeof settingsSchema>;
export type Theme = Settings['theme'];
