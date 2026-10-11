import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);
    const searchIds = query.q
        ? await searchDbIds(folderSearchDb, query.q, currentUser.id)
        : undefined;

    const where: Prisma.FolderWhereInput = {
        authorId: currentUser.id,
        ...(searchIds ? { id: { in: searchIds } } : {}),
    };

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.folder.findMany({ where, select: { id: true } }),
    );

    if (relevance) {
        const folders = await prisma.folder.findMany({
            where: { id: { in: relevance.ids } },
            include: folderInclude,
        });

        return {
            items: sortByIds(folders, relevance.ids).map((folder) =>
                serializeFolder(event, folder),
            ),
            total: relevance.total,
        };
    }

    const { field, dir } = parseSort(query.sort, ['createdAt', 'name', 'files'], 'createdAt:desc');

    const [inbox, total] = await Promise.all([
        prisma.folder.findFirst({
            where: { ...where, inbox: { isNot: null } },
            include: folderInclude,
        }),
        prisma.folder.count({ where }),
    ]);

    const showInbox = inbox && !query.offset;

    const folders = await prisma.folder.findMany({
        where: { ...where, inbox: { is: null } },
        include: folderInclude,
        orderBy: [field === 'files' ? { files: { _count: dir } } : { [field]: dir }, { id: 'asc' }],
        skip: inbox && query.offset ? query.offset - 1 : query.offset,
        take: showInbox ? query.limit - 1 : query.limit,
    });

    return {
        items: (showInbox ? [inbox, ...folders] : folders).map((folder) =>
            serializeFolder(event, folder),
        ),
        total,
    };
});
