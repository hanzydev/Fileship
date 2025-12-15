import { removeMultiple } from '@orama/orama';

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

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
