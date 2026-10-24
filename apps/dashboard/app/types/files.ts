import type { FileModel } from '#shared/prisma/models';

export type FileData = Omit<
    FileModel,
    'size' | 'embedding' | 'textEmbedding' | 'ocrText' | 'metadata'
> & {
    views: { total: number; today: number };
    size: { raw: string; formatted: string };
    directUrl: string;
    embedUrl: string;
    thumbnailUrl?: string | null;
    metadata: IMediaMetadata | null;
    folder?: { id: string; name: string } | null;
    embed?: IEmbed;
    access?: FolderAccess;
};

export type DuplicateGroup = {
    id: string;
    size: string;
    wasted: string;
    files: FileData[];
};

export type DuplicateSummary = {
    files: number;
    wasted: string;
};

export type FileSummary = {
    count: number;
    size: string;
    categories: Record<FileCategory, number>;
};
