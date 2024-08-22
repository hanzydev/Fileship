import consola from 'consola';
import dayjs from 'dayjs';
import { lowerFirst, titleCase } from 'scule';

import { sendByFilter } from '~~/server/plugins/socketIO';
import { isAdmin } from '~~/utils/user';

type CreateLogData = {
    action: string;
    message: string;
    system?: boolean;
};

const AllowedActions = ['Login', 'Create User', 'Update User', 'Delete User'];

export const createLog = async (
    event: any,
    { action, message, system }: CreateLogData,
) => {
    const logLevel = +(process.env.LOG_LEVEL ?? 2);
    if (
        logLevel === 0 ||
        (!AllowedActions.includes(action) && logLevel !== 2)
    ) {
        return;
    }

    const currentUser = event?.context?.user;
    const ip = event
        ? getRequestIP(event, { xForwardedFor: true }) || 'Unknown'
        : '::1';

    if (system) {
        consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${message}`);
    } else {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - User ${titleCase(currentUser!.username)}, ${lowerFirst(message)}`,
        );
    }

    const log = await prisma.log.create({
        data: {
            action,
            userId: system ? null : currentUser!.id,
            message,
            system,
            ip,
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
