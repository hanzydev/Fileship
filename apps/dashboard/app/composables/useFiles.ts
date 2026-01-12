import type { FileModel } from '#shared/prisma/models';

export type FileData = Omit<FileModel, 'size' | 'embedding' | 'textEmbedding' | 'ocrText'> & {
    views: { total: number; today: number };
    size: { raw: string; formatted: string };
    directUrl: string;
    embedUrl: string;
    thumbnailUrl?: string | null;
    embed?: IEmbed;
};

export const useFiles = () => {
    return useState<FileData[]>('files', () => []);
};
