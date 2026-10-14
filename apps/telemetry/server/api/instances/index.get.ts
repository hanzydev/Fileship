import { z } from 'zod';

const SORT_FIELDS = ['lastSeenAt', 'firstSeenAt', 'users', 'files', 'storage'];

const querySchema = z.object({
    limit: z.coerce.number().int().min(1).max(200).default(50),
    offset: z.coerce.number().int().min(0).default(0),
    q: z.string().trim().max(64).default(''),
    sort: z.string().default('lastSeenAt:desc'),
});

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const query = await getValidatedQuery(event, querySchema.safeParse);

    if (!query.success) {
        throw createError({
            status: 400,
            message: 'Invalid query',
        });
    }

    const { q, sort, offset, limit } = query.data;
    const [field, dir] = sort.split(':');

    const where = q
        ? {
              OR: [
                  { id: { startsWith: q.toLowerCase() } },
                  { version: { startsWith: q.replace(/^v/, '') } },
              ],
          }
        : {};

    const [instances, total] = await prisma.$transaction([
        prisma.instance.findMany({
            where,
            orderBy: [
                {
                    [SORT_FIELDS.includes(field!) ? field! : 'lastSeenAt']:
                        dir === 'asc' ? 'asc' : 'desc',
                },
                { id: 'asc' },
            ],
            skip: offset,
            take: limit,
        }),
        prisma.instance.count({ where }),
    ]);

    return {
        items: instances.map(serializeInstance),
        total,
        retentionDays: useRuntimeConfig().instanceRetentionDays,
    };
});
