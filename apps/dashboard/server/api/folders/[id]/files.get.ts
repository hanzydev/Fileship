import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            author: {
                select: {
                    domains: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            status: 404,
            message: 'Folder not found',
        });
    }

    const isOwner = findFolderById.authorId === currentUser?.id;

    if (!isOwner && !findFolderById.public) throw forbiddenError;

    const query = await readListQuery(event, {
        mode: fileListShape.mode,
        type: fileListShape.type,
    });
    const searchIds = query.q
        ? await searchFileIds(query.q, findFolderById.authorId, isOwner ? query.mode : 'fulltext')
        : undefined;

    const where: Prisma.FileWhereInput = {
        folderId: findFolderById.id,
        ...(isOwner ? {} : { password: null, piiDetected: false }),
        AND: [
            query.type.length ? { OR: query.type.map(fileCategoryWhere) } : {},
            searchIds ? { id: { in: searchIds } } : {},
        ],
    };

    const args = {
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
            password: true,
            maxViews: true,
            expiresAt: true,
            folderId: true,
            ...(isOwner ? {} : { caption: true, piiReasons: true, authorId: true }),
        },
        include: {
            _count: { select: { views: true } },
        },
    } satisfies Prisma.FileFindManyArgs;

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.file.findMany({ where, select: { id: true } }),
    );

    const [items, total] = relevance
        ? [
              sortByIds(
                  await prisma.file.findMany({ ...args, where: { id: { in: relevance.ids } } }),
                  relevance.ids,
              ),
              relevance.total,
          ]
        : await Promise.all([
              prisma.file.findMany({
                  ...args,
                  where,
                  orderBy: fileOrderBy(query.sort),
                  skip: query.offset,
                  take: query.limit,
              }),
              prisma.file.count({ where }),
          ]);

    const todayViews = isOwner ? await getTodayViews(items.map((file) => file.id)) : new Map();

    return {
        items: items.map((file) =>
            serializeFile(event, file, findFolderById.author.domains, todayViews.get(file.id)),
        ),
        total,
    };
});
