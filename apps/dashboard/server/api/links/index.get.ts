import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    linkShortenerOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);
    const { field, dir } = parseSort(query.sort, ['createdAt', 'code', 'clicks'], 'createdAt:desc');

    const where: Prisma.LinkWhereInput = {
        authorId: currentUser.id,
        ...(query.q
            ? {
                  OR: [
                      { code: { contains: query.q, mode: 'insensitive' } },
                      { url: { contains: query.q, mode: 'insensitive' } },
                  ],
              }
            : {}),
    };

    const [links, total] = await Promise.all([
        prisma.link.findMany({
            where,
            orderBy: [{ [field]: dir }, { id: 'asc' }],
            skip: query.offset,
            take: query.limit,
        }),
        prisma.link.count({ where }),
    ]);

    return {
        items: links.map((link) => serializeLink(event, link, currentUser.domains)),
        total,
    };
});
