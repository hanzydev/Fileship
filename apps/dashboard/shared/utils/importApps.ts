export const IMPORT_APPS = {
    zipline: 'Zipline',
    xbackbone: 'XBackBone',
    chibisafe: 'Chibisafe',
} as const;

export type ImportApp = keyof typeof IMPORT_APPS;
