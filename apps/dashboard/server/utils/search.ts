import consola from 'consola';

import { type AnyOrama, insert, remove, search } from '@orama/orama';

import type { Prisma } from '~~/generated/prisma/client';

const SEARCH_LIMIT = 1000;
const QUERY_CACHE_SIZE = 200;
const RRF_K = 60;

const CLIP_FLOOR = 0.065;
const CLIP_RATIO = 0.8;
const SEMANTIC_FLOOR = 0.42;
const SEMANTIC_RATIO = 0.65;
const FULLTEXT_RATIO = 0.3;

const queryEmbeddings = new Map<string, Promise<number[] | null>>();

const getQueryEmbedding = (key: string, create: () => Promise<number[] | null>) => {
    const cached = queryEmbeddings.get(key);

    if (cached) {
        queryEmbeddings.delete(key);
        queryEmbeddings.set(key, cached);
        return cached;
    }

    const embedding = create().catch((error) => {
        queryEmbeddings.delete(key);
        throw error;
    });

    queryEmbeddings.set(key, embedding);
    if (queryEmbeddings.size > QUERY_CACHE_SIZE)
        queryEmbeddings.delete(queryEmbeddings.keys().next().value!);

    return embedding;
};

const keepRelevant = (hits: { id: string; score: number }[], floor: number, ratio: number) => {
    const top = Math.max(0, ...hits.map((hit) => hit.score));

    return hits
        .filter((hit) => hit.score >= floor && hit.score >= top * ratio)
        .sort((a, b) => b.score - a.score)
        .map((hit) => hit.id);
};

const toWords = (text: string) =>
    text
        .toLocaleLowerCase()
        .split(/[^\p{L}\p{N}]+/u)
        .filter(Boolean);

const containsWord = (
    file: { fileName: string; caption?: string; ocrText?: string },
    words: string[],
) => {
    const fileName = file.fileName.toLocaleLowerCase();
    const text = new Set(toWords(`${file.caption ?? ''} ${file.ocrText ?? ''}`));

    return words.some((word) => fileName.includes(word) || text.has(word));
};

const fuseRanked = (lists: { ids: string[]; weight: number }[]) => {
    const scores = new Map<string, number>();

    for (const { ids, weight } of lists)
        ids.forEach((id, index) =>
            scores.set(id, (scores.get(id) ?? 0) + weight / (RRF_K + index + 1)),
        );

    return [...scores.entries()].sort((a, b) => b[1] - a[1]).map(([id]) => id);
};

export const fileSearchSelect = {
    id: true,
    authorId: true,
    fileName: true,
    mimeType: true,
    embedding: true,
    textEmbedding: true,
    caption: true,
    ocrText: true,
} satisfies Prisma.FileSelect;

export const toFileSearchDoc = (
    file: Prisma.FileGetPayload<{ select: typeof fileSearchSelect }>,
) => ({
    ...file,
    fileName: `${file.fileName} ${toWords(file.fileName).join(' ')}`,
    embedding: (file.embedding as number[]) || undefined,
    textEmbedding: (file.textEmbedding as number[]) || undefined,
    caption: file.caption || undefined,
    ocrText: file.ocrText || undefined,
});

let searchWrites = Promise.resolve();

export const upsertFileSearchEntry = (fileId: string) =>
    (searchWrites = searchWrites
        .then(async () => {
            const file = await prisma.file.findUnique({
                where: { id: fileId },
                select: fileSearchSelect,
            });

            try {
                await remove(fileSearchDb, fileId);
            } catch {
                //
            }

            if (file) await insert(fileSearchDb, toFileSearchDoc(file));
        })
        .catch(() => undefined));

export const searchDbIds = async (db: AnyOrama, term: string, authorId?: string) => {
    const searched = await search(db, {
        term,
        limit: SEARCH_LIMIT,
        ...(authorId ? { where: { authorId: { eq: authorId } } } : {}),
    });
    return searched.hits.map((hit) => hit.id);
};

const isSmartSearchEnabled = async (authorId: string) => {
    if (!(await isAiEnabled())) return false;

    const user = await prisma.user.findUnique({
        where: { id: authorId },
        select: { aiSettings: true },
    });

    return (user?.aiSettings as any)?.enabled ?? true;
};

export const searchFileIds = async (
    query: string,
    authorId: string,
    mode: 'vector' | 'fulltext' = 'fulltext',
) => {
    const where = { authorId: { eq: authorId } };

    const runFulltext = async (wholeWords = false) => {
        const { hits } = await search(fileSearchDb, {
            mode: 'fulltext',
            term: query,
            properties: ['fileName', 'caption', 'ocrText'],
            tolerance: 0,
            threshold: 0,
            limit: SEARCH_LIMIT,
            where,
            boost: {
                fileName: 4,
                caption: 2,
                ocrText: 1,
            },
        });

        const words = toWords(query);

        return keepRelevant(
            wholeWords ? hits.filter((hit) => containsWord(hit.document, words)) : hits,
            0,
            FULLTEXT_RATIO,
        );
    };

    if (mode === 'fulltext' || !(await isSmartSearchEnabled(authorId))) return runFulltext();

    const runClip = async () => {
        const vector = (await getQueryEmbedding(`clip:${query}`, () =>
            ai.createClipQueryEmbedding(query),
        ))!;
        const magnitude = Math.hypot(...vector);

        const { hits } = await search(fileSearchDb, {
            mode: 'vector',
            similarity: Math.min(1, CLIP_FLOOR / magnitude),
            limit: SEARCH_LIMIT,
            where,
            vector: { value: vector, property: 'embedding' },
        });

        return keepRelevant(
            hits.map((hit) => ({ id: hit.id, score: hit.score * magnitude })),
            CLIP_FLOOR,
            CLIP_RATIO,
        );
    };

    const runSemantic = async () => {
        const vector = await getQueryEmbedding(`text:${query}`, () =>
            ai.createSemanticTextEmbedding(query),
        );
        if (!vector) return [];

        const { hits } = await search(fileSearchDb, {
            mode: 'vector',
            similarity: SEMANTIC_FLOOR,
            limit: SEARCH_LIMIT,
            where,
            vector: { value: vector, property: 'textEmbedding' },
        });

        return keepRelevant(hits, SEMANTIC_FLOOR, SEMANTIC_RATIO);
    };

    const skipOnError = (error: any) => {
        consola.error(`Smart search failed: ${error?.message ?? error}`);
        return [];
    };

    const [semanticIds, clipIds, fulltextIds] = await Promise.all([
        runSemantic().catch(skipOnError),
        runClip().catch(skipOnError),
        runFulltext(true),
    ]);

    return fuseRanked([
        { ids: semanticIds, weight: 1 },
        { ids: clipIds, weight: 1 },
        { ids: fulltextIds, weight: 1.5 },
    ]);
};
