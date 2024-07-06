import type { User } from '@prisma/client';

import type { IEmbed, IUserLimits } from '~~/utils/types';

export type UserData = Omit<
    User,
    'password' | 'totpSecret' | 'embed' | 'limits'
> & {
    embed: IEmbed;
    limits: IUserLimits;
};

export const useUsers = () => {
    return useState<UserData[]>('users', () => []);
};
