import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);
    const searchIds = query.q
        ? await searchDbIds(noteSearchDb, query.q, currentUser.id)
        : undefined;

    const where: Prisma.NoteWhereInput = {
        authorId: currentUser.id,
        ...(searchIds ? { id: { in: searchIds } } : {}),
    };

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.note.findMany({ where, select: { id: true } }),
    );

    const { field, dir } = parseSort(query.sort, ['createdAt', 'title'], 'createdAt:desc');

    const [notes, total] = relevance
        ? [
              sortByIds(
                  await prisma.note.findMany({ where: { id: { in: relevance.ids } } }),
                  relevance.ids,
              ),
              relevance.total,
          ]
        : await Promise.all([
              prisma.note.findMany({
                  where,
                  orderBy: [{ [field]: dir }, { id: 'asc' }],
                  skip: query.offset,
                  take: query.limit,
              }),
              prisma.note.count({ where }),
          ]);

    return {
        items: notes.map((note) => ({
            ...note,
            publicUrl: note.public
                ? buildPublicUrl(event, currentUser.domains, `/note/${note.id}`)
                : undefined,
        })),
        total,
    };
});
