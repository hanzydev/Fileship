import type { FolderRole } from '#shared/prisma/enums';
import type { FolderModel } from '#shared/prisma/models';

export type FolderData = FolderModel & {
    fileCount: number;
    folderCount: number;
    memberCount?: number;
    access?: FolderAccess;
    owner?: { id: string; username: string; avatar: string | null };
    parent?: { id: string; name: string } | null;
    ancestors?: { id: string; name: string }[];
    covers?: { id: string; fileName: string; url: string }[];
    publicUrl?: string;
    isInbox: boolean;
};

export type FolderMemberData = {
    userId: string;
    username: string;
    avatar: string | null;
    role: FolderRole;
    createdAt: string;
};
