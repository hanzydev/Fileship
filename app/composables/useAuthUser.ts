import type { User } from '@prisma/client';

export type AuthUserData = Omit<
    User,
    'password' | 'totpSecret' | 'embed' | 'limits' | 'domains'
> & {
    currentSessionId: string;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
