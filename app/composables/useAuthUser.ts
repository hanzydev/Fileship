import type { User } from '@prisma/client';

import type { IEmbed } from '~~/utils/types';

export type AuthUserData = Omit<
    User,
    'password' | 'totpSecret' | 'embed' | 'limits'
> & {
    currentSessionId: string;
    embed: IEmbed;
};

export const useAuthUser = () => {
    return useState<AuthUserData | null>('authUser', () => null);
};
