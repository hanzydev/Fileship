import type { UserModel } from '#shared/prisma/models';

export type AuthUserData = Omit<
    UserModel,
    'password' | 'totpSecret' | 'embed' | 'domains' | 'aiSettings'
> & {
    currentSessionId: string;
    stats: UserBasicStats;
    aiSettings: IUserAiSettings;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
