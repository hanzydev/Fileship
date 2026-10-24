import { filesize } from 'filesize';

import type { Prisma } from '~~/generated/prisma/client';

const storageWarningSelect = {
    id: true,
    username: true,
    email: true,
    language: true,
    permissions: true,
    superAdmin: true,
    limits: true,
    storageWarnedAt: true,
} satisfies Prisma.UserSelect;

type StorageWarningUser = Prisma.UserGetPayload<{ select: typeof storageWarningSelect }>;

const pendingChecks = new Map<string, Language | undefined>();

const clearStorageWarning = (userId: string) =>
    prisma.user.update({ where: { id: userId }, data: { storageWarnedAt: null } });

const updateStorageWarning = async (user: StorageWarningUser, used: bigint, hint?: Language) => {
    const limit = usableSpaceOf(user);

    if (!limit || used * 10n < limit * 9n) {
        if (user.storageWarnedAt) await clearStorageWarning(user.id);
        return;
    }

    if (user.storageWarnedAt || !user.email) return;

    const { count } = await prisma.user.updateMany({
        where: { id: user.id, storageWarnedAt: null },
        data: { storageWarnedAt: new Date() },
    });

    if (!count) return;

    const language = toLanguage(user.language) ?? hint ?? 'en';
    const size = (value: bigint) =>
        filesize(value, { base: 2, standard: 'jedec', locale: language });

    await sendEmail({
        to: user.email,
        language,
        template: 'storageWarning',
        params: {
            username: user.username,
            used: size(used),
            limit: size(limit),
            percent: new Intl.NumberFormat(language, {
                style: 'percent',
                maximumFractionDigits: 0,
            }).format(Number((used * 100n) / limit) / 100),
        },
        path: '/dashboard/settings/storage',
    }).catch(() => clearStorageWarning(user.id));
};

export const checkStorageWarning = async (userId: string, hint?: Language) => {
    if (!(await isEmailEnabled())) return;

    const user = await prisma.user.findUnique({
        where: { id: userId },
        select: storageWarningSelect,
    });

    if (!user) return;

    await updateStorageWarning(
        user,
        usableSpaceOf(user) === null ? 0n : await storageUsedBy(user.id),
        hint,
    );
};

export const queueStorageCheck = (userId: string, hint?: Language) => {
    if (pendingChecks.has(userId)) return;

    pendingChecks.set(userId, hint);

    setTimeout(() => {
        pendingChecks.delete(userId);
        checkStorageWarning(userId, hint).catch(() => null);
    }, 2_000);
};

export const checkStorageWarnings = async () => {
    if (!(await isEmailEnabled())) return 0;

    const users = (await prisma.user.findMany({ select: storageWarningSelect })).filter(
        (user) => user.storageWarnedAt || (user.email && usableSpaceOf(user) !== null),
    );

    const used = await storageUsedByUsers(users.map((user) => user.id));

    for (const user of users) await updateStorageWarning(user, BigInt(used.get(user.id) ?? 0));

    return users.length;
};
