import { z } from 'zod';

import { removeMultiple } from '@orama/orama';

const validationSchema = z
    .object({
        verificationData: z.any().optional(),
    })
    .optional();

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) {
        throw createError({
            statusCode: 400,
            message: 'Invalid body',
            data: { formErrors: body.error.format() },
        });
    }

    await verifySession(event, body.data?.verificationData);

    const logs = await prisma.log.findMany({
        select: { id: true },
    });

    await prisma.log.deleteMany({});

    await removeMultiple(
        logSearchDb,
        logs.map((l) => l.id),
    );

    await sendByFilter(isAdmin, 'log:deleteAll', null);

    await createLog(event, {
        action: 'Flush Logs',
        message: 'Flushed all logs',
    });
});
