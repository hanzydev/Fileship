import type { UserPermission } from '@prisma/client';

import type { IEmbed, IUserLimits } from '~~/utils/types';
import { isAdmin } from '~~/utils/user';

export default defineEventHandler((event) => {
    if (!isAdmin(event)) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return prisma.user.findMany({
        orderBy: {
            createdAt: 'desc',
        },
        select: {
            id: true,
            username: true,
            avatar: true,
            permissions: true,
            totpEnabled: true,
            superAdmin: true,
            limits: true,
            embed: true,
            createdAt: true,
            _count: {
                select: { files: true, notes: true, codes: true, urls: true },
            },
        },
    }) as unknown as Promise<
        {
            id: string;
            username: string;
            avatar: string | null;
            permissions: UserPermission[];
            totpEnabled: boolean;
            superAdmin: boolean;
            embed: IEmbed;
            limits: IUserLimits;
            createdAt: Date;
            _count: {
                files: number;
                notes: number;
                codes: number;
                urls: number;
            };
        }[]
    >;
});
