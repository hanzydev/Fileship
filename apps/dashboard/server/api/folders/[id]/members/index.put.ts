import { z } from 'zod';

import { FolderRole } from '#shared/prisma/enums';

const validationSchema = z.object({
    username: z.string().trim().min(1, 'errorsContent.shareUsernameRequired'),
    role: z.enum(FolderRole),
});

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) throw invalidBodyError(body.error);

    const { folder } = await findFolderWithAccess(
        getRouterParam(event, 'id'),
        currentUser.id,
        'owner',
    );

    const user = await prisma.user.findUnique({
        where: { username: body.data.username },
        select: { id: true },
    });

    if (!user) throw apiError(404, 'errorsContent.shareUserNotFound');
    if (user.id === currentUser.id) throw apiError(400, 'errorsContent.shareWithYourself');

    const member = await prisma.folderMember.upsert({
        where: { folderId_userId: { folderId: folder.id, userId: user.id } },
        create: { folderId: folder.id, userId: user.id, role: body.data.role },
        update: { role: body.data.role },
        include: { user: { select: { username: true, avatar: true } } },
    });

    await createLog(event, {
        action: 'Share Folder',
        message: `Shared folder ${folder.name} with ${member.user.username} as ${member.role}`,
    });

    for (const userId of [currentUser.id, user.id]) {
        sendToUser(userId, 'folder:share', { folderId: folder.id });
    }

    return serializeFolderMember(member);
});
