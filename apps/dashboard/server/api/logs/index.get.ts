import { z } from 'zod';

import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const query = await readListQuery(event, { action: z.string().optional() });
    const searchIds = query.q ? await searchDbIds(logSearchDb, query.q) : undefined;

    const where: Prisma.LogWhereInput = {
        ...(query.action ? { action: query.action } : {}),
        ...(searchIds ? { id: { in: searchIds } } : {}),
    };

    const select = {
        id: true,
        ip: true,
        action: true,
        userId: true,
        user: {
            select: {
                id: true,
                username: true,
                avatar: true,
            },
        },
        message: true,
        system: true,
        createdAt: true,
    } satisfies Prisma.LogSelect;

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.log.findMany({ where, select: { id: true } }),
    );

    const { field, dir } = parseSort(query.sort, ['createdAt', 'action', 'user'], 'createdAt:desc');

    const [items, total] = relevance
        ? [
              sortByIds(
                  await prisma.log.findMany({ where: { id: { in: relevance.ids } }, select }),
                  relevance.ids,
              ),
              relevance.total,
          ]
        : await Promise.all([
              prisma.log.findMany({
                  where,
                  select,
                  orderBy: [
                      field === 'user' ? { user: { username: dir } } : { [field]: dir },
                      { id: 'asc' },
                  ],
                  skip: query.offset,
                  take: query.limit,
              }),
              prisma.log.count({ where }),
          ]);

    return { items, total };
});
