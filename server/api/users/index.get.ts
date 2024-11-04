import type { UserPermission } from '@prisma/client';

export default defineEventHandler((event) => {
    adminOnly(event);

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
            createdAt: true,
            domains: true,
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
            limits: IUserLimits;
            domains: string[];
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
