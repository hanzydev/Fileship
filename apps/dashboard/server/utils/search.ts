import consola from 'consola';

import { type AnyOrama, insert, remove, search } from '@orama/orama';

import type { AIPreset, SearchCutoff } from './aiPresets';
import type { VectorKind } from './vectorIndex';

import type { Prisma } from '~~/generated/prisma/client';

const SEARCH_LIMIT = 1000;
const QUERY_CACHE_SIZE = 200;
const RRF_K = 60;

const FULLTEXT_RATIO = 0.3;
const QUERY_EMBEDDING_TIMEOUT_MS = 10_000;

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
    caption: true,
    ocrText: true,
} satisfies Prisma.FileSelect;

const fileVectorSelect = {
    id: true,
    authorId: true,
    embedding: true,
    embeddingModel: true,
    textEmbedding: true,
    textEmbeddingModel: true,
} satisfies Prisma.FileSelect;

export const toFileSearchDoc = (
    file: Prisma.FileGetPayload<{ select: typeof fileSearchSelect }>,
) => ({
    ...file,
    fileName: `${file.fileName} ${toWords(file.fileName).join(' ')}`,
    caption: file.caption || undefined,
    ocrText: file.ocrText || undefined,
});

const indexFileVectors = (
    file: Prisma.FileGetPayload<{ select: typeof fileVectorSelect }>,
    preset: AIPreset,
) =>
    setFileVectors(file.id, file.authorId, {
        image: file.embeddingModel === preset.image.key ? (file.embedding as number[]) : null,
        text: file.textEmbeddingModel === preset.text.key ? (file.textEmbedding as number[]) : null,
    });

export const reloadFileVectors = async () => {
    const preset = await getAIPreset();
    let cursor: string | undefined;

    clearFileVectors();

    while (true) {
        const files = await prisma.file.findMany({
            where: {
                OR: [{ embeddingModel: preset.image.key }, { textEmbeddingModel: preset.text.key }],
            },
            select: fileVectorSelect,
            orderBy: { id: 'asc' },
            take: 500,
            ...(cursor ? { cursor: { id: cursor }, skip: 1 } : {}),
        });

        files.forEach((file) => indexFileVectors(file, preset));
        if (files.length < 500) break;
        cursor = files.at(-1)!.id;
    }
};

let searchWrites = Promise.resolve();

export const upsertFileSearchEntry = (fileId: string) =>
    (searchWrites = searchWrites
        .then(async () => {
            const file = await prisma.file.findUnique({
                where: { id: fileId },
                select: { ...fileSearchSelect, ...fileVectorSelect },
            });

            try {
                await remove(fileSearchDb, fileId);
            } catch {
                //
            }

            removeFileVectors([fileId]);

            if (!file) return;

            await insert(fileSearchDb, toFileSearchDoc(file));
            indexFileVectors(file, await getAIPreset());
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

    const preset = await getAIPreset();

    const runVectors = async (
        kind: VectorKind,
        cutoff: SearchCutoff,
        embed: () => Promise<number[] | null>,
    ) => {
        const vector = await Promise.race([
            getQueryEmbedding(`${kind}:${preset[kind].key}:${query}`, embed),
            new Promise<null>((resolve) =>
                setTimeout(() => resolve(null), QUERY_EMBEDDING_TIMEOUT_MS),
            ),
        ]);
        if (!vector) return [];

        return keepRelevant(
            searchFileVectors(authorId, kind, vector, cutoff.floor),
            cutoff.floor,
            cutoff.ratio,
        );
    };

    const skipOnError = (error: any) => {
        consola.error(`Smart search failed: ${error?.message ?? error}`);
        return [];
    };

    const [textIds, imageIds, fulltextIds] = await Promise.all([
        runVectors('text', preset.text.cutoff, () => ai.embedText(query, true)).catch(skipOnError),
        runVectors('image', preset.image.cutoff, () => ai.embedImageQuery(query)).catch(
            skipOnError,
        ),
        runFulltext(true),
    ]);

    return fuseRanked([
        { ids: textIds, weight: 1 },
        { ids: imageIds, weight: 1 },
        { ids: fulltextIds, weight: 1.5 },
    ]);
};
