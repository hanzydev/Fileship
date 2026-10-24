export const FILE_NAME_TYPES = ['Random', 'UUID', 'Original'] as const;

export type FileNameType = (typeof FILE_NAME_TYPES)[number];

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

export type Settings = {
    instanceUrl: string;
    token: string;
    connection: ConnectionInfo | null;
    fileNameType: FileNameType;
    expiration: number;
};

export const settingsItem = storage.defineItem<Settings>('local:settings', {
    fallback: {
        instanceUrl: '',
        token: '',
        connection: null,
        fileNameType: 'Random',
        expiration: 0,
    },
});

export const updateSettings = async (patch: Partial<Settings>) =>
    settingsItem.setValue({ ...(await settingsItem.getValue()), ...patch });
