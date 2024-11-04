import type { User } from '@prisma/client';

export type UserData = Omit<
    User,
    'password' | 'totpSecret' | 'embed' | 'limits' | 'backupRestoreState' | 'theme'
> & {
    limits: IUserLimits;
};

export const useUsers = () => {
    return useState<UserData[]>('users', () => []);
};
