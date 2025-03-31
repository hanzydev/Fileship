import type { File } from '@prisma/client';

export type FileData = Omit<File, 'size'> & {
    views: { total: number; today: number };
    size: { raw: string; formatted: string };
    directUrl: string;
    embedUrl: string;
    thumbnailUrl?: string | null;
};

export const useFiles = () => {
    return useState<FileData[]>('files', () => []);
};
