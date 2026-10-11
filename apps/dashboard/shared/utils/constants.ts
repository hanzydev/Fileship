import type { IEmbed, IUserAiSettings, IUserLimits } from './types';

export const defaultEmbed: IEmbed = {
    title: '{fileName}',
    description: '',
    siteName: '',
    color: '#5865f2',
    enabled: false,
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
    { label: 'Never', value: null },
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
