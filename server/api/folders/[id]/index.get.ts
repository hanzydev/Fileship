import { defu } from 'defu';
import { filesize } from 'filesize';

import { defaultEmbed } from '~~/utils/constants';
import type { IEmbed } from '~~/utils/types';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
        },
        include: {
            files: true,
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

    const reqUrl = getRequestURL(event);

    const protocol = process.env.RETURN_HTTPS
        ? process.env.RETURN_HTTPS === 'true'
            ? 'https'
            : 'http'
        : reqUrl.protocol.slice(0, -1);

    const domain = findFolderById.author.domains.length
        ? findFolderById.author.domains[
              Math.floor(Math.random() * findFolderById.author.domains.length)
          ]
        : reqUrl.host;

    return {
        ...findFolderById,
        author: undefined,
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
                directUrl: `${protocol}://${domain}/u/${file.fileName}`,
                embedUrl: `${protocol}://${domain}/view/${file.fileName}`,
            })),
        embed: defu(findFolderById.author.embed, defaultEmbed) as IEmbed,
        publicUrl: findFolderById.public
            ? `${protocol}://${domain}/folder/${findFolderById.id}`
            : undefined,
    };
});
