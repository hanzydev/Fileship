import { filesize } from 'filesize';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: true,
        },
    });

    if (!findFolderById) {
        throw createError({
            statusCode: 404,
            statusMessage: 'Not Found',
            message: 'Folder not found',
        });
    }

    if (findFolderById.authorId !== currentUser?.id) {
        if (!findFolderById.public) {
            throw createError({
                statusCode: 403,
                statusMessage: 'Forbidden',
                message: 'You do not have permission to perform this action',
            });
        } else {
            findFolderById.files = findFolderById.files.filter(
                (f) => !f.password,
            );
        }
    }

    return {
        ...findFolderById,
        files: findFolderById.files
            .sort((a, b) => b.createdAt.getTime() - a.createdAt.getTime())
            .map((file) => ({
                ...file,
                password: undefined,
                maxViews: undefined,
                expiresAt: undefined,
                folderId: undefined,
                size: {
                    raw: file.size.toString(),
                    formatted: filesize(file.size.toString()),
                },
            })),
    };
});
