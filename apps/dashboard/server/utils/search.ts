import { type AnyOrama, search } from '@orama/orama';

const SEARCH_LIMIT = 1000;

const mergeRanked = (lists: { ids: string[]; weight: number }[]) => {
    const scores = new Map<string, number>();

    for (const { ids, weight } of lists) {
        ids.forEach((id, index) => scores.set(id, (scores.get(id) ?? 0) + weight / (index + 1)));
    }

    return [...scores.entries()].sort((a, b) => b[1] - a[1]).map(([id]) => id);
};

export const searchDbIds = async (db: AnyOrama, term: string, authorId?: string) => {
    const searched = await search(db, {
        term,
        limit: SEARCH_LIMIT,
        ...(authorId ? { where: { authorId: { eq: authorId } } } : {}),
    });
    return searched.hits.map((hit) => hit.id);
};

export const searchFileIds = async (
    query: string,
    authorId: string,
    mode: 'vector' | 'fulltext' = 'fulltext',
) => {
    const runFulltext = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'fulltext',
            term: query,
            tolerance: 0,
            threshold: 0,
            limit: SEARCH_LIMIT,
            where: { authorId: { eq: authorId } },
            boost: {
                fileName: 4,
                caption: 2,
                ocrText: 1,
            },
        });

        return searched.hits.filter((h) => h.score > 3).map((h) => h.id);
    };

    if (mode === 'fulltext' || !(await isAiEnabled())) return runFulltext();

    const runClipVector = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'vector',
            similarity: 0.2,
            limit: SEARCH_LIMIT,
            where: { authorId: { eq: authorId } },
            vector: {
                value: await ai.createTextEmbedding(query),
                property: 'embedding',
            },
        });

        return searched.hits.map((h) => h.id);
    };

    const runSemanticVector = async () => {
        const searched = await search(fileSearchDb, {
            mode: 'vector',
            similarity: 0.5,
            limit: SEARCH_LIMIT,
            where: { authorId: { eq: authorId } },
            vector: {
                value:
                    (await ai.createSemanticTextEmbedding(query)) ??
                    new Array(ai.SEMANTIC_TEXT_EMBEDDING_DIM).fill(0),
                property: 'textEmbedding',
            },
        });

        return searched.hits.map((h) => h.id);
    };

    const [clipIds, semanticIds, exactTextIds] = await Promise.all([
        runClipVector(),
        runSemanticVector(),
        runFulltext(),
    ]);

    return mergeRanked([
        { ids: semanticIds, weight: 0.4 },
        { ids: clipIds, weight: 0.3 },
        { ids: exactTextIds, weight: 0.3 },
    ]);
};
