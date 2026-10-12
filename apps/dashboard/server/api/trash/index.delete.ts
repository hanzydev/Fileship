import { z } from 'zod';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data?.verificationData);

    const trashed = { authorId: currentUser.id, deletedAt: { not: null } };
    const { files, folders, notes } = await deleteTrashed({
        files: trashed,
        folders: trashed,
        notes: trashed,
    });

    await createLog(event, {
        action: 'Empty Trash',
        message: `Deleted ${files.length} files, ${folders.length} folders and ${notes.length} notes forever`,
    });

    sendToUser(currentUser.id, 'trash:update', null);
});
