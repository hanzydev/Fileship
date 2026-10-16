import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        name: z
            .string()
            .min(1, 'errorsContent.folderNameMin')
            .max(32, 'errorsContent.folderNameMax')
            .optional(),
        public: z.boolean().optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            authorId: currentUser.id,
            deletedAt: null,
        },
    });

    if (!findFolderById) {
        throw apiError(404, 'errorsContent.folderNotFound');
    }

    const _updatedFolder = await prisma.folder.update({
        where: {
            id: folderId,
        },
        include: folderInclude,
        data: {
            name: body.data.name,
            public: body.data.public,
        },
    });

    const updatedFolder = serializeFolder(event, _updatedFolder);

    await update(folderSearchDb, _updatedFolder.id, {
        id: updatedFolder.id,
        authorId: updatedFolder.authorId,
        name: updatedFolder.name,
    });

    await createLog(event, {
        action: 'Update Folder',
        message: `Updated folder ${findFolderById.name}`,
    });

    sendToUser(currentUser.id, 'folder:update', updatedFolder);

    return updatedFolder;
});
