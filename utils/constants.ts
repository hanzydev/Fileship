import type { IEmbed, IUserLimits } from './types';

export const defaultEmbed: IEmbed = {
    title: '{fileName}',
    description: '',
    siteName: '',
    color: '#5e58f9',
    enabled: false,
};

export const defaultUserLimits: IUserLimits = {
    backupLimit: -1,
    usableSpace: -1,
};
