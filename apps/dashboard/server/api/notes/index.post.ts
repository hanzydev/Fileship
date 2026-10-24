import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object({
    title: z
        .string()
        .min(3, 'errorsContent.noteTitleMin')
        .max(48, 'errorsContent.noteTitleMax')
        .transform((title) => title.trim()),
    content: z
        .string()
        .min(1, 'errorsContent.noteContentMin')
        .max(50_000, 'errorsContent.noteContentMax'),
    public: z.boolean().optional().default(false),
});

export default defineEventHandler(async (event) => {
    noteTakerOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const _note = await prisma.note.create({
        data: {
            title: body.data.title,
            content: body.data.content,
            public: body.data.public,
            authorId: currentUser.id,
        },
    });

    const note = {
        ..._note,
        publicUrl: _note.public
            ? buildPublicUrl(event, currentUser.domains, `/note/${_note.id}`)
            : undefined,
    };

    await insert(noteSearchDb, {
        id: note.id,
        authorId: note.authorId,
        title: note.title,
    });

    await createLog(event, {
        action: 'Take Note',
        message: `Took note ${note.title}`,
    });

    emitNoteCreated(event, currentUser.id, note);

    sendToUser(currentUser.id, 'note:create', note);

    return note;
});
