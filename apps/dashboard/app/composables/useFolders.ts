import type { FolderModel } from '#shared/prisma/models';

export type FolderData = FolderModel & {
    files: string[];
    publicUrl?: string;
    isInbox: boolean;
};

export const useFolders = () => {
    return useState<FolderData[]>('folders', () => []);
};
