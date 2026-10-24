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

    await assertFolderParent(currentUser.id, body.data.parentId);

    if (body.data.files?.length) {
        const files = await prisma.file.findMany({
            where: {
                id: {
                    in: body.data.files,
                },
                authorId: currentUser.id,
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
            authorId: currentUser.id,
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

    sendToUser(currentUser.id, 'folder:create', folder);

    return folder;
});
