import type { Prisma } from '~~/generated/prisma/client';

const getSummary = async (where: Prisma.FileWhereInput) => {
    const [aggregate, ...counts] = await Promise.all([
        prisma.file.aggregate({ where, _count: true, _sum: { size: true } }),
        ...FILE_CATEGORY_NAMES.map((category) =>
            prisma.file.count({ where: { AND: [where, fileCategoryWhere(category)] } }),
        ),
    ]);

    return {
        count: aggregate._count,
        size: (aggregate._sum.size ?? 0n).toString(),
        categories: Object.fromEntries(
            FILE_CATEGORY_NAMES.map((category, index) => [category, counts[index]!]),
        ),
    };
};

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readFileListQuery(event);

    const shared = await sharedFolderOf(query.folder, currentUser.id);

    const { base, where, searchIds } = await buildFileWhere(
        shared?.authorId ?? currentUser.id,
        query,
    );

    const args = {
        include: {
            folder: { select: { id: true, name: true } },
            _count: { select: { views: true } },
        },
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
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

    const todayViews = await getTodayViews(items.map((file) => file.id));

    return {
        items: items.map((file) => ({
            ...serializeFile(
                event,
                file,
                shared?.author.domains ?? currentUser.domains,
                todayViews.get(file.id),
            ),
            access: shared?.access,
        })),
        total,
        summary: query.summary && !query.offset ? await getSummary(base) : undefined,
    };
});
