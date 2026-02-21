import { z } from 'zod';

import { search } from '@orama/orama';

const validationSchema = z.object({
    query: z
        .string()
        .min(1, 'Search query must be at least 1 character long')
        .max(100, 'Search query must be at most 100 characters long'),
    mode: z.enum(['vector', 'fulltext']),
});

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: {
                orderBy: {
                    createdAt: 'desc',
                },
                select: {
                    id: true,
                    password: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            message: 'Folder not found',
        });
    }

    if (findFolderById.authorId !== currentUser?.id) {
        if (!findFolderById.public) {
            throw forbiddenError;
        } else {
            findFolderById.files = findFolderById.files.filter((f) => !f.password);
        }
    }

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    if (!AI_ENABLED && body.data.mode !== 'fulltext') body.data.mode = 'fulltext';

    const mergeRanked = (lists: { ids: string[]; weight: number }[]) => {
        const scores = new Map<string, number>();

        for (const { ids, weight } of lists) {
            for (let i = 0; i < ids.length; i++) {
                const id = ids[i];
                if (!id) continue;
                const rankScore = 1 / (i + 1);
                scores.set(id, (scores.get(id) ?? 0) + weight * rankScore);
            }
        }

        return [...scores.entries()].sort((a, b) => b[1] - a[1]).map(([id]) => id);
    };

    const runFulltext = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'fulltext',
            term: body.data.query,
            tolerance: 0,
            threshold: 0,
            boost: {
                fileName: 4,
                caption: 2,
                ocrText: 1,
            },
        });

        return searched.hits.filter((h) => h.score > 3).map((h) => h.id);
    };

    const runClipVector = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'vector',
            similarity: 0.2,
            vector: {
                value: await ai.createTextEmbedding(body.data.query),
                property: 'embedding',
            },
        });

        return searched.hits.map((h) => h.id);
    };

    const runSemanticVector = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'vector',
            similarity: 0.5,
            vector: {
                value:
                    (await ai.createSemanticTextEmbedding(body.data.query)) ??
                    new Array(ai.SEMANTIC_TEXT_EMBEDDING_DIM).fill(0),
                property: 'textEmbedding',
            },
        });

        return searched.hits.map((h) => h.id);
    };

    let ids: string[] = [];

    if (body.data.mode === 'fulltext') {
        ids = await runFulltext();
    } else {
        const [clipIds, semanticIds, exactTextIds] = await Promise.all([
            runClipVector(),
            runSemanticVector(),
            runFulltext(),
        ]);

        ids = mergeRanked([
            { ids: semanticIds, weight: 0.4 },
            { ids: clipIds, weight: 0.3 },
            { ids: exactTextIds, weight: 0.3 },
        ]);
    }

    const allowedIds = new Set(findFolderById.files.map((f) => f.id));
    return ids.filter((id) => allowedIds.has(id));
});
