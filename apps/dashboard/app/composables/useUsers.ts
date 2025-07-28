import type { User } from '@prisma/client';

export type UserBasicStats = {
    files: number;
    folders: number;
    notes: number;
    codes: number;
};

export type UserData = Omit<
    User,
    'password' | 'totpSecret' | 'embed' | 'backupRestoreState' | 'theme'
> & {
    limits: IUserLimits;
    stats: UserBasicStats;
};

export const useUsers = () => {
    return useState<UserData[]>('users', () => []);
};
