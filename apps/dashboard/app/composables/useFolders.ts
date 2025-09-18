import type { FolderModel } from '#shared/prisma/models';

export type FolderData = FolderModel & {
    files: string[];
    publicUrl?: string;
};

export const useFolders = () => {
    return useState<FolderData[]>('folders', () => []);
};
