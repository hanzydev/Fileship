import { z } from 'zod';

const validationSchema = z.object(
    {
        title: z
            .string({
                invalid_type_error: 'Invalid title',
                required_error: 'Missing title',
            })
            .min(3, 'Title must be at least 3 characters')
            .max(48, 'Title must be at most 48 characters')
            .transform((title) => title.trim()),
        content: z
            .string({
                invalid_type_error: 'Invalid content',
                required_error: 'Missing content',
            })
            .min(1, 'Content must be at least 1 character')
            .max(50_000, 'Content must be at most 50000 characters'),
    },
    { invalid_type_error: 'Invalid body', required_error: 'Missing body' },
);

export default defineEventHandler(async (event) => {
    noteTakerOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw createError({
            statusCode: 400,
            statusMessage: 'Bad Request',
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

    await createLog(event, {
        action: 'Take Note',
        message: `Took note ${note.title}`,
    });

    sendToUser(currentUser.id, 'create:note', note);

    return note;
});
