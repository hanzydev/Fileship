import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            deletedAt: null,
        },
        select: { id: true, authorId: true, public: true },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const access = await folderAccessOf(findFolderById, currentUser?.id);
    const isOwner = !!access;

    if (!isOwner && !findFolderById.public) throw forbiddenError();

    const query = await readListQuery(event);
    const { field, dir } = parseSort(query.sort, ['createdAt', 'name', 'files'], 'name:asc');

    const where: Prisma.FolderWhereInput = {
        parentId: findFolderById.id,
        deletedAt: null,
        ...(!isOwner && { public: true }),
    };

    const page = {
        where,
        orderBy: [field === 'files' ? { files: { _count: dir } } : { [field]: dir }, { id: 'asc' }],
        skip: query.offset,
        take: query.limit,
    } satisfies Prisma.FolderFindManyArgs;

    const [items, total] = await Promise.all([
        isOwner
            ? prisma.folder.findMany({ ...page, include: folderInclude }).then((folders) =>
                  Promise.all(
                      folders.map(async (folder) => ({
                          ...serializeFolder(event, folder),
                          access:
                              access === 'owner'
                                  ? undefined
                                  : await folderAccessOf(folder, currentUser!.id),
                      })),
                  ),
              )
            : prisma.folder.findMany({
                  ...page,
                  select: { id: true, name: true, createdAt: true },
              }),
        prisma.folder.count({ where }),
    ]);

    return { items, total };
});
