import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        title: z
            .string()
            .min(3, 'Title must be at least 3 characters')
            .max(48, 'Title must be at most 48 characters')
            .optional()
            .transform((title) => title?.trim()),
        content: z
            .string()
            .min(1, 'Content must be at least 1 character')
            .max(50_000, 'Content must be at most 50000 characters')
            .optional(),
    })
    .strict();

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
            authorId: currentUser.id,
        },
    });

    if (!findNoteById) {
        throw createError({
            statusCode: 404,
            message: 'Note not found',
        });
    }

    const updatedNote = await prisma.note.update({
        where: {
            id: noteId,
        },
        data: body.data,
    });

    await update(noteSearchDb, updatedNote.id, {
        id: updatedNote.id,
        title: updatedNote.title,
    });

    await createLog(event, {
        action: 'Update Note',
        message: `Updated note ${findNoteById.title}`,
    });

    sendToUser(currentUser.id, 'update:note', updatedNote);

    return updatedNote;
});
