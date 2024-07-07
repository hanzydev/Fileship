import { sendByFilter } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

type CreateLogData = {
    action: string;
    message: string;
    system?: boolean;
};

export const createLog = async (
    event: any,
    { action, message, system }: CreateLogData,
) => {
    const currentUser = event.context.user;

    const log = await prisma.log.create({
        data: {
            action,
            userId: system ? null : currentUser!.id,
            message,
            system,
            ip: getRequestIP(event, { xForwardedFor: true }) || 'Unknown',
        },
        select: {
            ip: true,
            action: true,
            user: {
                select: {
                    id: true,
                    username: true,
                    avatar: true,
                },
            },
            message: true,
            system: true,
            createdAt: true,
        },
    });

    await sendByFilter(
        (socket) => isAdmin(socket.handshake.auth.user)!,
        'create:log',
        log,
    );
};
