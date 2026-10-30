import type { IEmbed, IUserAiSettings, IUserLimits } from './types';

export const defaultEmbed: IEmbed = {
    title: '{fileName}',
    description: '',
    siteName: '',
    color: '#5865f2',
    enabled: false,
    discord: { enabled: true, file: null, note: null, folder: null },
};

export const defaultUserLimits: IUserLimits = {
    backupLimit: -1,
    usableSpace: -1,
};

export const defaultUserAiSettings: IUserAiSettings = {
    enabled: true,
    suppressPii: true,
};

export const EXPIRATIONS: { label: string; value: number | null }[] = [
    { label: 'labels.expiration.never', value: null },
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

export const MAX_FOLDER_DEPTH = 10;
