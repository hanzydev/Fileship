import { FolderRole } from '#shared/prisma/enums';
import type { FolderMember } from '~~/generated/prisma/client';

const ACCESS_RANK: Record<FolderAccess, number> = { viewer: 1, editor: 2, owner: 3 };

const folderChain = async (folderId: string) => [
    folderId,
    ...(await folderAncestors(folderId)).map((folder) => folder.id),
];

export const folderAccessOf = async (
    folder: { id: string; authorId: string },
    userId: string | undefined,
): Promise<FolderAccess | null> => {
    if (!userId) return null;
    if (folder.authorId === userId) return 'owner';

    const memberships = await prisma.folderMember.findMany({
        where: { userId, folderId: { in: await folderChain(folder.id) } },
        select: { role: true },
    });

    if (!memberships.length) return null;

    return memberships.some((membership) => membership.role === FolderRole.Editor)
        ? 'editor'
        : 'viewer';
};

export const hasFolderAccess = (access: FolderAccess | null, needed: FolderAccess) =>
    !!access && ACCESS_RANK[access] >= ACCESS_RANK[needed];

export const findFolderWithAccess = async (
    folderId: string | undefined,
    userId: string | undefined,
    needed: FolderAccess,
) => {
    const folder = await prisma.folder.findUnique({ where: { id: folderId, deletedAt: null } });
    const access = folder && (await folderAccessOf(folder, userId));

    if (!folder || !access) throw apiError(404, 'errorsContent.folderNotFound');
    if (!hasFolderAccess(access, needed)) throw forbiddenError();

    return { folder, access };
};

export const sharedFolderOf = async (folderId: string | undefined, userId: string) => {
    const folder =
        folderId && isUuid(folderId)
            ? await prisma.folder.findUnique({
                  where: { id: folderId, deletedAt: null },
                  include: { author: { select: { domains: true } } },
              })
            : null;
    const access = folder && folder.authorId !== userId && (await folderAccessOf(folder, userId));

    return access ? { ...folder!, access } : null;
};

export const fileAccessOf = async (
    file: { authorId: string; folderId: string | null },
    userId: string | undefined,
) => {
    if (userId && file.authorId === userId) return 'owner';
    if (!userId || !file.folderId) return null;

    return folderAccessOf({ id: file.folderId, authorId: file.authorId }, userId);
};

export const folderAudience = async (
    ownerId: string,
    folderIds: (string | null)[],
    subtreeIds: string[] = [],
) => {
    const ids = folderIds.filter((id): id is string => !!id);
    const chains = (await Promise.all(ids.map(folderChain))).flat();
    const members = [...chains, ...subtreeIds].length
        ? await prisma.folderMember.findMany({
              where: { folderId: { in: [...chains, ...subtreeIds] } },
              select: { userId: true },
          })
        : [];

    return [...new Set([ownerId, ...members.map((member) => member.userId)])];
};

export const sendToFolder = async (
    ownerId: string,
    folderIds: (string | null)[],
    event: string,
    data: unknown,
) => {
    for (const userId of await folderAudience(ownerId, folderIds)) sendToUser(userId, event, data);
};

export const serializeFolderMember = (
    member: FolderMember & { user: { username: string; avatar: string | null } },
) => ({
    userId: member.userId,
    username: member.user.username,
    avatar: member.user.avatar,
    role: member.role,
    createdAt: member.createdAt,
});
