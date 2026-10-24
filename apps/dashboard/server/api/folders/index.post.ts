import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object({
    name: z.string().min(1, 'errorsContent.folderNameMin').max(32, 'errorsContent.folderNameMax'),
    files: z.array(z.string()).max(5000).optional(),
    public: z.boolean().optional().default(false),
    parentId: z.string().nullish(),
});

export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const ownerId = body.data.parentId
        ? (await findFolderWithAccess(body.data.parentId, currentUser.id, 'editor')).folder.authorId
        : currentUser.id;

    if (ownerId !== currentUser.id && body.data.public) throw forbiddenError();

    await assertFolderParent(ownerId, body.data.parentId);

    if (body.data.files?.length) {
        const files = await prisma.file.findMany({
            where: {
                id: {
                    in: body.data.files,
                },
                authorId: ownerId,
                deletedAt: null,
            },
        });

        if (files.length !== body.data.files.length) {
            throw apiError(400, 'errorsContent.invalidFiles');
        }
    }

    const _folder = await prisma.folder.create({
        data: {
            name: body.data.name,
            public: body.data.public,
            parentId: body.data.parentId || null,
            files: {
                connect: body.data.files?.map((file) => ({ id: file })),
            },
            authorId: ownerId,
        },
        include: folderInclude,
    });

    const folder = serializeFolder(event, _folder);

    await insert(folderSearchDb, {
        id: folder.id,
        authorId: folder.authorId,
        name: folder.name,
    });

    await createLog(event, {
        action: 'Create Folder',
        message: `Created folder ${folder.name}`,
    });

    await sendToFolder(ownerId, [folder.id], 'folder:create', folder);

    return folder;
});
