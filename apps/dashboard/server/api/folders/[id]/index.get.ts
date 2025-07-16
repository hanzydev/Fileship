import { existsSync } from 'node:fs';

import { defu } from 'defu';
import { filesize } from 'filesize';
import { join } from 'pathe';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: {
                orderBy: {
                    createdAt: 'desc',
                },
                omit: {
                    embedding: true,
                },
            },
            author: {
                select: {
                    domains: true,
                    embed: true,
                },
            },
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
            throw forbiddenError;
        } else {
            findFolderById.files = findFolderById.files.filter((f) => !f.password);
        }
    }

    return {
        ...findFolderById,
        author: undefined,
        files: findFolderById.files.map((file) => ({
            ...file,
            password: undefined,
            maxViews: undefined,
            expiresAt: undefined,
            folderId: undefined,
            size: {
                raw: file.size.toString(),
                formatted: filesize(file.size),
            },
            directUrl: buildPublicUrl(event, findFolderById.author.domains, `/u/${file.fileName}`),
            embedUrl: buildPublicUrl(
                event,
                findFolderById.author.domains,
                `/view/${file.fileName}`,
            ),
            thumbnailUrl: file.mimeType.startsWith('video/')
                ? existsSync(join(dataDirectory, 'thumbnails', `${file.id}.jpeg`))
                    ? buildPublicUrl(
                          event,
                          findFolderById.author.domains,
                          `/u/${file.fileName}/thumbnail`,
                      )
                    : null
                : undefined,
        })),
        embed: defu(findFolderById.author.embed, defaultEmbed) as IEmbed,
        publicUrl: findFolderById.public
            ? buildPublicUrl(event, findFolderById.author.domains, `/folder/${findFolderById.id}`)
            : undefined,
    };
});
