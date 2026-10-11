import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const query = await readListQuery(event);
    const searchIds = query.q ? await searchDbIds(userSearchDb, query.q) : undefined;

    const where: Prisma.UserWhereInput = searchIds ? { id: { in: searchIds } } : {};

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.user.findMany({ where, select: { id: true } }),
    );

    const { field, dir } = parseSort(
        query.sort,
        ['createdAt', 'username', 'files', 'notes'],
        'createdAt:desc',
    );

    const [users, total] = relevance
        ? [
              sortByIds(
                  await prisma.user.findMany({
                      where: { id: { in: relevance.ids } },
                      select: userListSelect,
                  }),
                  relevance.ids,
              ),
              relevance.total,
          ]
        : await Promise.all([
              prisma.user.findMany({
                  where,
                  select: userListSelect,
                  orderBy: [
                      field === 'files' || field === 'notes'
                          ? { [field]: { _count: dir } }
                          : { [field]: dir },
                      { id: 'asc' },
                  ],
                  skip: query.offset,
                  take: query.limit,
              }),
              prisma.user.count({ where }),
          ]);

    return { items: users.map(serializeListUser), total };
});
