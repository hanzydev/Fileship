const TREE_GUARD = 64;

export interface FolderNode {
    id: string;
    name: string;
    parentId: string | null;
    public: boolean;
    deletedAt: Date | null;
    level: number;
}

export const folderAncestors = async (folderId: string) =>
    (
        await prisma.$queryRaw<FolderNode[]>`
            WITH RECURSIVE chain AS (
                SELECT p."id", p."name", p."parentId", p."public", p."deletedAt", 1 AS "level"
                FROM "Folder" f JOIN "Folder" p ON p."id" = f."parentId"
                WHERE f."id" = ${folderId}
                UNION ALL
                SELECT p."id", p."name", p."parentId", p."public", p."deletedAt", c."level" + 1
                FROM chain c JOIN "Folder" p ON p."id" = c."parentId"
                WHERE c."level" < ${TREE_GUARD}
            )
            SELECT * FROM chain
        `
    ).reverse();

export const folderSubtree = async (folderIds: string[]) => {
    if (!folderIds.length) return [];

    const rows = await prisma.$queryRaw<FolderNode[]>`
        WITH RECURSIVE tree AS (
            SELECT "id", "name", "parentId", "public", "deletedAt", 0 AS "level"
            FROM "Folder"
            WHERE "id" = ANY(${folderIds})
            UNION ALL
            SELECT f."id", f."name", f."parentId", f."public", f."deletedAt", t."level" + 1
            FROM tree t JOIN "Folder" f
                ON f."parentId" = t."id" AND f."deletedAt" IS NOT DISTINCT FROM t."deletedAt"
            WHERE t."level" < ${TREE_GUARD}
        )
        SELECT * FROM tree ORDER BY "level"
    `;

    const seen = new Set<string>();

    return rows.filter((row) => !seen.has(row.id) && seen.add(row.id));
};

export const assertFolderParent = async (
    authorId: string,
    parentId: string | null | undefined,
    folderId?: string,
) => {
    if (!parentId) return;

    const parent = await prisma.folder.findUnique({
        where: { id: parentId, authorId, deletedAt: null },
        select: { id: true },
    });

    if (!parent) throw apiError(404, 'errorsContent.folderNotFound');

    const subtree = folderId ? await folderSubtree([folderId]) : [];

    if (subtree.some((node) => node.id === parentId)) {
        throw apiError(400, 'errorsContent.folderIntoItself');
    }

    const height = subtree.length ? Math.max(...subtree.map((node) => node.level)) + 1 : 1;
    const depth = (await folderAncestors(parentId)).length + 1;

    if (depth + height > MAX_FOLDER_DEPTH) {
        throw apiError(400, 'errorsContent.folderTooDeep', { max: MAX_FOLDER_DEPTH });
    }
};
