export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const query = await readListQuery(event);
    const { groups, summary } = await findDuplicateGroups(
        currentUser.id,
        query.offset,
        query.limit,
    );

    const files = await prisma.file.findMany({
        where: {
            authorId: currentUser.id,
            deletedAt: null,
            contentHash: { in: groups.map((group) => group.contentHash) },
        },
        include: {
            folder: { select: { id: true, name: true } },
            _count: { select: { views: true } },
        },
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
        },
        orderBy: { createdAt: 'asc' },
    });

    const todayViews = await getTodayViews(files.map((file) => file.id));
    const byHash = Map.groupBy(files, (file) => file.contentHash!);

    return {
        items: groups.map((group) => ({
            id: group.contentHash,
            size: group.size.toString(),
            wasted: (group.size * BigInt(group.count - 1)).toString(),
            files: (byHash.get(group.contentHash) ?? []).map((file) =>
                serializeFile(event, file, currentUser.domains, todayViews.get(file.id)),
            ),
        })),
        total: summary.groups,
        summary: { files: summary.files, wasted: summary.wasted.toString() },
    };
});
