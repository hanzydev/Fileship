import type { UserModel } from '#shared/prisma/models';

export type UserBasicStats = {
    files: number;
    folders: number;
    notes: number;
};

export type UserData = Omit<
    UserModel,
    'password' | 'totpSecret' | 'embed' | 'backupRestoreState' | 'theme'
> & {
    limits: IUserLimits;
    stats: UserBasicStats;
};

export const useUsers = () => {
    return useState<UserData[]>('users', () => []);
};
