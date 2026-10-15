import { defu } from 'defu';
import { filesize } from 'filesize';
import type { H3Event } from 'h3';
import { z } from 'zod';

import type { Prisma } from '~~/generated/prisma/client';

export const listQuerySchema = z.object({
    limit: z.coerce.number().int().min(1).max(200).default(50),
    offset: z.coerce.number().int().min(0).default(0),
    q: z.string().trim().max(100).optional(),
    sort: z.string().optional(),
});

export const readListQuery = async <T extends z.ZodRawShape>(event: H3Event, shape?: T) => {
    const result = await getValidatedQuery(event, listQuerySchema.extend(shape || {}).safeParse);

    if (!result.success) {
        throw createError({
            status: 400,
            message: 'Invalid query',
            data: { formErrors: result.error.format() },
        });
    }

    return result.data as z.infer<typeof listQuerySchema> & z.infer<z.ZodObject<T>>;
};

export const parseSort = (sort: string | undefined, fields: string[], fallback: string) => {
    const [field, dir] = (sort || fallback).split(':');

    return {
        field: fields.includes(field!) ? field! : fallback.split(':')[0]!,
        dir: (dir === 'asc' ? 'asc' : 'desc') as Prisma.SortOrder,
    };
};

export const sortByIds = <T extends { id: string }>(items: T[], ids: string[]) => {
    const order = new Map(ids.map((id, index) => [id, index]));
    return items.sort((a, b) => order.get(a.id)! - order.get(b.id)!);
};

export const findRelevancePage = async (
    query: { offset: number; limit: number; sort?: string },
    searchIds: string[] | undefined,
    findMatches: () => Promise<{ id: string }[]>,
) => {
    if (!searchIds || query.sort) return;

    const ids = sortByIds(await findMatches(), searchIds).map((item) => item.id);

    return { total: ids.length, ids: ids.slice(query.offset, query.offset + query.limit) };
};

const media: Prisma.FileWhereInput[] = [
    { mimeType: { startsWith: 'image/' } },
    { mimeType: { startsWith: 'video/' } },
    { mimeType: { startsWith: 'audio/' } },
];

const documentMime: Prisma.FileWhereInput = { mimeType: { in: DOCUMENT_FILE_MIME_TYPES } };
const archiveMime: Prisma.FileWhereInput = { mimeType: { in: ARCHIVE_FILE_MIME_TYPES } };
const codeExtension: Prisma.FileWhereInput = {
    OR: CODE_FILE_EXTENSIONS.map((extension) => ({
        fileName: { endsWith: extension, mode: 'insensitive' as const },
    })),
};

const categories: Record<string, Prisma.FileWhereInput> = {
    image: media[0]!,
    video: media[1]!,
    audio: media[2]!,
    document: {
        NOT: media,
        OR: [
            documentMime,
            { mimeType: { startsWith: 'text/' }, NOT: [archiveMime, codeExtension] },
        ],
    },
    archive: { NOT: [...media, documentMime], ...archiveMime },
    code: { NOT: [...media, documentMime, archiveMime], ...codeExtension },
};

export const fileCategoryWhere = (category: string): Prisma.FileWhereInput =>
    category === 'other' ? { NOT: Object.values(categories) } : categories[category]!;

export const FILE_CATEGORY_NAMES = [...Object.keys(categories), 'other'];

export const serializeFile = <
    T extends {
        id: string;
        fileName: string;
        mimeType: string;
        size: bigint;
        _count: { views: number };
    },
>(
    event: H3Event,
    { _count, size, ...file }: T,
    domains: string[],
    todayViews = 0,
) => ({
    ...file,
    size: {
        raw: size.toString(),
        formatted: filesize(size),
    },
    views: {
        total: _count.views,
        today: todayViews,
    },
    directUrl: buildPublicUrl(event, domains, `/u/${file.fileName}`),
    embedUrl: buildPublicUrl(event, domains, `/view/${file.fileName}`),
    thumbnailUrl: getThumbnailUrl(event, domains, file),
});

export const getTodayViews = async (fileIds: string[]) => {
    const start = new Date();
    start.setUTCHours(0, 0, 0, 0);

    const views = await prisma.view.groupBy({
        by: ['fileId'],
        where: { fileId: { in: fileIds }, createdAt: { gte: start } },
        _count: true,
    });

    return new Map(views.map((view) => [view.fileId!, view._count]));
};

const csv = z
    .string()
    .optional()
    .transform((value) => (value ? value.split(',').filter(Boolean) : []));

const categoryCsv = csv.transform((value) =>
    value.filter((category) => FILE_CATEGORY_NAMES.includes(category)),
);

export const fileListShape = {
    folder: z.string().optional(),
    excludeFolder: z.string().optional(),
    type: categoryCsv,
    ids: csv,
    mode: z.enum(['vector', 'fulltext']).default('fulltext'),
    summary: z.stringbool().default(false),
};

export const FILE_SORT_FIELDS = ['createdAt', 'name', 'fileName', 'size', 'views'];

export const readFileListQuery = (event: H3Event) => readListQuery(event, fileListShape);

export const buildFileWhere = async (
    authorId: string,
    query: Awaited<ReturnType<typeof readFileListQuery>>,
) => {
    const base: Prisma.FileWhereInput = {
        authorId,
        deletedAt: null,
        ...(query.folder && query.folder !== 'all'
            ? { folderId: query.folder === 'none' ? null : query.folder }
            : {}),
        ...(query.excludeFolder
            ? { OR: [{ folderId: null }, { folderId: { not: query.excludeFolder } }] }
            : {}),
    };

    const searchIds = query.q ? await searchFileIds(query.q, authorId, query.mode) : undefined;

    const where: Prisma.FileWhereInput = {
        ...base,
        AND: [
            query.type.length ? { OR: query.type.map(fileCategoryWhere) } : {},
            query.ids.length ? { id: { in: query.ids } } : {},
            searchIds ? { id: { in: searchIds } } : {},
        ],
    };

    return { base, where, searchIds };
};

export const fileOrderBy = (sort: string | undefined): Prisma.FileOrderByWithRelationInput[] => {
    const { field, dir } = parseSort(sort, FILE_SORT_FIELDS, 'createdAt:desc');

    return [field === 'views' ? { views: { _count: dir } } : { [field]: dir }, { id: 'asc' }];
};

export const folderInclude = {
    inbox: { select: { id: true } },
    author: { select: { domains: true } },
    _count: { select: { files: { where: { deletedAt: null } } } },
    files: {
        where: {
            deletedAt: null,
            piiDetected: false,
            OR: [{ mimeType: { startsWith: 'image/' } }, { mimeType: { startsWith: 'video/' } }],
        },
        orderBy: { createdAt: 'desc' },
        take: 8,
        select: { id: true, fileName: true, mimeType: true },
    },
} satisfies Prisma.FolderInclude;

export const serializeFolder = (
    event: H3Event,
    folder: Prisma.FolderGetPayload<{ include: typeof folderInclude }>,
) => ({
    ...folder,
    inbox: undefined,
    author: undefined,
    _count: undefined,
    files: undefined,
    fileCount: folder._count.files,
    covers: folder.files
        .filter((file) => file.mimeType.startsWith('image/') || hasThumbnail(file.id))
        .slice(0, 4)
        .map((file) => ({
            id: file.id,
            fileName: file.fileName,
            url: buildPublicUrl(
                event,
                folder.author.domains,
                hasThumbnail(file.id) ? `/u/${file.fileName}/thumbnail` : `/u/${file.fileName}`,
            ),
        })),
    publicUrl: folder.public
        ? buildPublicUrl(event, folder.author.domains, `/folder/${folder.id}`)
        : undefined,
    isInbox: !!folder.inbox,
});

export const userCountSelect = {
    files: { where: { deletedAt: null } },
    folders: { where: { deletedAt: null } },
    notes: { where: { deletedAt: null } },
} satisfies Prisma.UserCountOutputTypeSelect;

export const userListSelect = {
    id: true,
    username: true,
    avatar: true,
    permissions: true,
    totpEnabled: true,
    superAdmin: true,
    limits: true,
    createdAt: true,
    domains: true,
    _count: {
        select: userCountSelect,
    },
} satisfies Prisma.UserSelect;

export const serializeListUser = (
    user: Prisma.UserGetPayload<{ select: typeof userListSelect }>,
) => ({
    ...user,
    _count: undefined,
    limits: defu(user.limits, defaultUserLimits) as IUserLimits,
    stats: user._count,
});
