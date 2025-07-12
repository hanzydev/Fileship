import type { User } from '@prisma/client';

export type AuthUserData = Omit<User, 'password' | 'totpSecret' | 'embed' | 'domains'> & {
    currentSessionId: string;
    stats: UserBasicStats;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
