import type { UserModel } from '#shared/prisma/models';

export type AuthUserData = Omit<UserModel, 'password' | 'totpSecret' | 'embed' | 'domains'> & {
    currentSessionId: string;
    stats: UserBasicStats;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
