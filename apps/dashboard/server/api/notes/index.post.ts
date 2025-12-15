import { z } from 'zod';

import { insert } from '@orama/orama';

const validationSchema = z.object({
    title: z
        .string()
        .min(3, 'Title must be at least 3 characters')
        .max(48, 'Title must be at most 48 characters')
        .transform((title) => title.trim()),
    content: z
        .string()
        .min(1, 'Content must be at least 1 character')
        .max(50_000, 'Content must be at most 50000 characters'),
});

export default defineEventHandler(async (event) => {
    noteTakerOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    const note = await prisma.note.create({
        data: {
            title: body.data.title,
            content: body.data.content,
            authorId: currentUser.id,
        },
    });

    await insert(noteSearchDb, {
        id: note.id,
        title: note.title,
    });

    await createLog(event, {
        action: 'Take Note',
        message: `Took note ${note.title}`,
    });

    sendToUser(currentUser.id, 'note:create', note);

    return note;
});
