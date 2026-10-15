import consola from 'consola';
import dayjs from 'dayjs';
import type { H3Event } from 'h3';
import { lowerFirst, titleCase } from 'scule';

import { insert } from '@orama/orama';

type CreateLogData = {
    action: string;
    message: string;
    system?: boolean;
};

const AllowedActions = [
    'Login',
    'Register',
    'TOTP',
    'Add Passkey',
    'Delete Passkey',
    'Revoke Session',
    'Impersonate',
    'Regenerate Upload Token',
    'Create User',
    'Update User',
    'Delete User',
    'Update Settings',
    'Create Backup',
    'Upload Backup',
    'Load Backup',
    'Load Backup Failed',
    'Delete Backup',
    'Delete From Trash',
    'Empty Trash',
    'Restore From Trash',
    'Create Storage',
    'Update Storage',
    'Delete Storage',
    'Retry AI Jobs',
    'Reset AI Data',
    'Update Fileship',
    'Flush Logs',
];

export const createLog = async (
    event: H3Event | null,
    { action, message, system }: CreateLogData,
) => {
    const { logLevel } = await getSettings();
    if (logLevel === 0 || (!AllowedActions.includes(action) && logLevel !== 2)) {
        return;
    }

    const currentUser = event?.context?.user;
    const ip = event ? getClientIp(event) : '::1';

    if (system || !currentUser) {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${message}${ip !== '::1' ? ` from ${ip}` : ''}`,
        );
    } else {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - User ${titleCase(currentUser!.username)}, ${lowerFirst(message)} from ${ip}`,
        );
    }

    const log = await prisma.log.create({
        data: {
            action,
            userId: system ? null : currentUser?.id,
            message,
            system,
            ip,
        },
        select: {
            id: true,
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

    await insert(logSearchDb, {
        id: log.id,
        action: log.action,
        message: log.message,
        ip: log.ip,
    });

    await sendByFilter(isAdmin, 'log:create', log);
};
