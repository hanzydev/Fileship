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
        parentId: z
            .string()
            .nullish()
            .transform((value) => (value === '' ? null : value)),
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

    const { folder: findFolderById, access } = await findFolderWithAccess(
        folderId,
        currentUser.id,
        'editor',
    );

    if (
        access !== 'owner' &&
        (body.data.public !== undefined || body.data.parentId !== undefined)
    ) {
        throw forbiddenError();
    }

    const isMoved =
        body.data.parentId !== undefined && body.data.parentId !== findFolderById.parentId;

    if (isMoved) await assertFolderParent(currentUser.id, body.data.parentId, findFolderById.id);

    const _updatedFolder = await prisma.folder.update({
        where: {
            id: folderId,
        },
        include: folderInclude,
        data: {
            name: body.data.name,
            public: body.data.public,
            parentId: isMoved ? body.data.parentId : undefined,
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

    await sendToFolder(
        findFolderById.authorId,
        [findFolderById.id, isMoved ? findFolderById.parentId : null],
        'folder:update',
        isMoved ? updatedFolder : { ...updatedFolder, parentId: undefined, parent: undefined },
    );

    return updatedFolder;
});
