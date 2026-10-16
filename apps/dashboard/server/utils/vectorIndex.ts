export type VectorKind = 'image' | 'text';

type Entry = {
    authorId: string;
    vectors: Partial<Record<VectorKind, Float32Array>>;
};

const entries = new Map<string, Entry>();
const authorFiles = new Map<string, Set<string>>();

export const removeFileVectors = (ids: string[]) => {
    for (const id of ids) {
        const entry = entries.get(id);
        if (!entry) continue;

        entries.delete(id);
        authorFiles.get(entry.authorId)?.delete(id);
    }
};

export const setFileVectors = (
    id: string,
    authorId: string,
    vectors: Partial<Record<VectorKind, number[] | null>>,
) => {
    removeFileVectors([id]);

    const stored = Object.fromEntries(
        Object.entries(vectors)
            .filter(([, vector]) => vector?.length)
            .map(([kind, vector]) => [kind, Float32Array.from(vector!)]),
    );

    if (!Object.keys(stored).length) return;

    entries.set(id, { authorId, vectors: stored });
    if (!authorFiles.has(authorId)) authorFiles.set(authorId, new Set());
    authorFiles.get(authorId)!.add(id);
};

export const clearFileVectors = () => {
    entries.clear();
    authorFiles.clear();
};

export const searchFileVectors = (
    authorId: string,
    kind: VectorKind,
    query: number[],
    minScore: number,
) => {
    const hits: { id: string; score: number }[] = [];

    for (const id of authorFiles.get(authorId) ?? []) {
        const vector = entries.get(id)!.vectors[kind];
        if (!vector || vector.length !== query.length) continue;

        let score = 0;
        for (let i = 0; i < vector.length; i++) score += vector[i]! * query[i]!;

        if (score >= minScore) hits.push({ id, score });
    }

    return hits.sort((a, b) => b.score - a.score);
};
