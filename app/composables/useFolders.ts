import type { Folder } from '@prisma/client';

export type FolderData = Folder & { files: { id: string }[] };

export const useFolders = () => {
    return useState<FolderData[]>('folders', () => []);
};
