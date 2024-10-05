import { sendByFilter } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/permissions';

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    await prisma.log.deleteMany({});

    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'delete:log:all',
        null,
    );

    await createLog(event, {
        action: 'Flush Logs',
        message: 'Flushed all logs',
    });
});
