import { randomBytes } from 'node:crypto';

import type { EmailTokenType } from '#shared/prisma/enums';

export const createEmailToken = async (
    userId: string,
    type: EmailTokenType,
    email: string,
    lifetime: number,
) => {
    const token = randomBytes(32).toString('base64url');

    await prisma.$transaction([
        prisma.emailToken.deleteMany({ where: { userId, type } }),
        prisma.emailToken.create({
            data: {
                tokenHash: hashToken(token),
                type,
                email,
                userId,
                expiresAt: new Date(Date.now() + lifetime),
            },
        }),
    ]);

    return token;
};

export const findEmailToken = (token: string, type: EmailTokenType) =>
    prisma.emailToken.findFirst({
        where: { tokenHash: hashToken(token), type, expiresAt: { gt: new Date() } },
        include: { user: true },
    });

export const consumeEmailToken = async (id: string) => {
    const { count } = await prisma.emailToken.deleteMany({
        where: { id, expiresAt: { gt: new Date() } },
    });

    return count === 1;
};

export const updateUserEmail = async (
    userId: string,
    data: { email?: string | null; pendingEmail: string | null },
) => {
    const user = await prisma.user.update({
        where: { id: userId },
        data,
        select: { id: true, email: true, pendingEmail: true },
    });

    await refreshSocketUser(user.id, user);
    sendToUser(user.id, 'currentUser:update', user);
    await sendByFilter(isAdmin, 'user:update', user);

    return user;
};
