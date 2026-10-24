export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const query = await readFileListQuery(event);
    const shared = await sharedFolderOf(query.folder, currentUser.id);
    const { where } = await buildFileWhere(shared?.authorId ?? currentUser.id, query);

    const files = await prisma.file.findMany({
        where,
        orderBy: fileOrderBy(query.sort),
        select: { id: true },
    });

    return files.map((file) => file.id);
});
