import { z } from 'zod';

import { update } from '@orama/orama';

const validationSchema = z
    .object({
        title: z
            .string()
            .min(3, 'errorsContent.noteTitleMin')
            .max(48, 'errorsContent.noteTitleMax')
            .optional()
            .transform((title) => title?.trim()),
        content: z
            .string()
            .min(1, 'errorsContent.noteContentMin')
            .max(50_000, 'errorsContent.noteContentMax')
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

    const noteId = getRouterParam(event, 'id');

    const findNoteById = await prisma.note.findUnique({
        where: {
            id: noteId,
            authorId: currentUser.id,
            deletedAt: null,
        },
    });

    if (!findNoteById) {
        throw apiError(404, 'errorsContent.noteNotFound');
    }

    const _updatedNote = await prisma.note.update({
        where: {
            id: noteId,
        },
        data: body.data,
    });

    const updatedNote = {
        ..._updatedNote,
        publicUrl: _updatedNote.public
            ? buildPublicUrl(event, currentUser.domains, `/note/${_updatedNote.id}`)
            : null,
    };

    await update(noteSearchDb, updatedNote.id, {
        id: updatedNote.id,
        authorId: updatedNote.authorId,
        title: updatedNote.title,
    });

    await createLog(event, {
        action: 'Update Note',
        message: `Updated note ${findNoteById.title}`,
    });

    sendToUser(currentUser.id, 'note:update', updatedNote);

    return updatedNote;
});
