import type { FolderModel } from '#shared/prisma/models';

export type FolderData = FolderModel & {
    fileCount: number;
    covers?: { id: string; fileName: string; url: string }[];
    publicUrl?: string;
    isInbox: boolean;
};
