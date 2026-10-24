import type { UserModel } from '#shared/prisma/models';

import type { UserBasicStats } from '~/types/users';

export type AuthUserData = Omit<
    UserModel,
    | 'password'
    | 'totpSecret'
    | 'embed'
    | 'domains'
    | 'aiSettings'
    | 'limits'
    | 'backupRestoreStartedAt'
    | 'storageWarnedAt'
> & {
    currentSessionId: string;
    impersonator?: string | null;
    stats: UserBasicStats;
    aiSettings: IUserAiSettings;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
