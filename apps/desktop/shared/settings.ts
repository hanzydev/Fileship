import { z } from 'zod';

export const HOTKEY_ACTIONS = [
    'captureFullscreen',
    'captureRegion',
    'captureWindow',
    'captureMonitor',
    'captureLastRegion',
    'recordVideo',
    'recordGif',
    'uploadClipboard',
    'uploadFile',
    'shortenClipboard',
    'openMain',
] as const;

export type HotkeyAction = (typeof HOTKEY_ACTIONS)[number];

export const HOTKEY_LABELS: Record<HotkeyAction, string> = {
    captureFullscreen: 'Capture entire screen',
    captureRegion: 'Capture region',
    captureWindow: 'Capture active window',
    captureMonitor: 'Capture active monitor',
    captureLastRegion: 'Capture last region',
    recordVideo: 'Start or stop screen recording',
    recordGif: 'Start or stop GIF recording',
    uploadClipboard: 'Upload from clipboard',
    uploadFile: 'Upload file',
    shortenClipboard: 'Shorten URL from clipboard',
    openMain: 'Open Fileship',
};

export const DEFAULT_HOTKEYS: Record<HotkeyAction, string> = {
    captureFullscreen: 'PrintScreen',
    captureRegion: 'Control+PrintScreen',
    captureWindow: 'Alt+PrintScreen',
    captureMonitor: '',
    captureLastRegion: '',
    recordVideo: 'Shift+PrintScreen',
    recordGif: 'Control+Shift+PrintScreen',
    uploadClipboard: '',
    uploadFile: '',
    shortenClipboard: '',
    openMain: '',
};

export const FILE_NAME_TYPES = ['Random', 'UUID', 'Original'] as const;

export const EXPIRATIONS = [
    { label: 'Never', value: 0 },
    { label: '5 Minutes', value: 300_000 },
    { label: '15 Minutes', value: 900_000 },
    { label: '30 Minutes', value: 1_800_000 },
    { label: '1 Hour', value: 3_600_000 },
    { label: '6 Hours', value: 21_600_000 },
    { label: '12 Hours', value: 43_200_000 },
    { label: '1 Day', value: 86_400_000 },
    { label: '1 Week', value: 604_800_000 },
    { label: '2 Weeks', value: 1_209_600_000 },
    { label: '1 Month', value: 2_592_000_000 },
    { label: '3 Months', value: 7_776_000_000 },
    { label: '6 Months', value: 15_552_000_000 },
    { label: '1 Year', value: 31_536_000_000 },
];

export const COMPRESSIONS = [
    { label: 'None', value: 0 },
    { label: 'Low (25%)', value: 25 },
    { label: 'Medium (50%)', value: 50 },
    { label: 'High (75%)', value: 75 },
    { label: 'Maximum (100%)', value: 100 },
];

export const settingsSchema = z.object({
    startAtLogin: z.boolean().default(true),
    theme: z.enum(['system', 'light', 'dark']).default('system'),
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
            quickCrop: z.boolean().default(false),
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
    save: z
        .object({
            folder: z.string().default(''),
            pattern: z.string().min(1).max(128).default('%yyyy-%mo-%d_%h-%mi-%s'),
        })
        .prefault({}),
});

export type Settings = z.output<typeof settingsSchema>;
export type Theme = Settings['theme'];
