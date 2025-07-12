import type { IEmbed, IUserLimits } from './types';

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
