export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    await prisma.log.deleteMany({});

    await sendByFilter(isAdmin, 'delete:log:all', null);

    await createLog(event, {
        action: 'Flush Logs',
        message: 'Flushed all logs',
    });
});
