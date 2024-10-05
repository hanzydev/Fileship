import { isAdmin } from '~~/utils/permissions';

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    await prisma.log.deleteMany({});

    await sendByFilter((user) => isAdmin(user), 'delete:log:all', null);

    await createLog(event, {
        action: 'Flush Logs',
        message: 'Flushed all logs',
    });
});
