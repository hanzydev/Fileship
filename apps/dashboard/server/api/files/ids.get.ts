export default defineEventHandler(async (event) => {
    userOnly(event);

    const query = await readFileListQuery(event);
    const { where } = await buildFileWhere(event.context.user!.id, query);

    const files = await prisma.file.findMany({
        where,
        orderBy: fileOrderBy(query.sort),
        select: { id: true },
    });

    return files.map((file) => file.id);
});
