import type { Prisma } from '~~/generated/prisma/client';

export const inviteSelect = {
    id: true,
    email: true,
    permissions: true,
    usableSpace: true,
    expiresAt: true,
    createdAt: true,
    invitedBy: { select: { username: true } },
} satisfies Prisma.InviteSelect;

export const findInvite = (token: string) =>
    prisma.invite.findFirst({
        where: { tokenHash: hashToken(token), expiresAt: { gt: new Date() } },
        select: { ...inviteSelect, invitedById: true },
    });
