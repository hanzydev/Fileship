import type { FolderModel } from '#shared/prisma/models';

export type FolderData = FolderModel & {
    fileCount: number;
    folderCount: number;
    parent?: { id: string; name: string } | null;
    ancestors?: { id: string; name: string }[];
    covers?: { id: string; fileName: string; url: string }[];
    publicUrl?: string;
    isInbox: boolean;
};
