import { hash } from 'argon2';
import { defu } from 'defu';

import { insert } from '@orama/orama';

import type { UserPermission } from '#shared/prisma/enums';

export const createUser = async (data: {
    username: string;
    password: string;
    permissions?: UserPermission[];
    superAdmin?: boolean;
    limits?: Partial<IUserLimits>;
}) => {
    const _createdUser = await prisma.user.create({
        data: {
            username: data.username,
            password: await hash(data.password),
            permissions: data.permissions,
            superAdmin: data.superAdmin,
            limits: defu(data.limits, defaultUserLimits),
        },
        select: {
            id: true,
            username: true,
            avatar: true,
            permissions: true,
            totpEnabled: true,
            createdAt: true,
            limits: true,
            superAdmin: true,
            _count: {
                select: { files: true, folders: true, notes: true },
            },
        },
    });

    const inboxFolder = await prisma.folder.create({
        data: {
            name: 'Inbox',
            public: false,
            authorId: _createdUser.id,
            inbox: {
                create: {
                    userId: _createdUser.id,
                },
            },
        },
    });

    await insert(folderSearchDb, {
        id: inboxFolder.id,
        authorId: inboxFolder.authorId,
        name: inboxFolder.name,
    });

    const createdUser = {
        ..._createdUser,
        _count: undefined,
        stats: _createdUser._count,
        limits: defu(_createdUser.limits, defaultUserLimits) as IUserLimits,
    };

    await insert(userSearchDb, {
        id: createdUser.id,
        username: createdUser.username,
    });

    await sendByFilter(isAdmin, 'user:create', createdUser);

    return createdUser;
};
