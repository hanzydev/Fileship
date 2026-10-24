import consola from 'consola';
import dayjs from 'dayjs';
import type { H3Event } from 'h3';
import { lowerFirst, titleCase } from 'scule';

import { insert } from '@orama/orama';

type CreateLogData = {
    action: string;
    message: string;
    system?: boolean;
    user?: { id: string; username: string };
};

const AllowedActions = [
    'Login',
    'Register',
    'Reset Password',
    'TOTP',
    'Add Passkey',
    'Delete Passkey',
    'Revoke Session',
    'Impersonate',
    'Regenerate Upload Token',
    'Create User',
    'Create Invite',
    'Delete Invite',
    'Update User',
    'Delete User',
    'Update Settings',
    'Create Backup',
    'Create Backup Failed',
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
    'Import Files',
    'Retry AI Jobs',
    'Reset AI Data',
    'Update Fileship',
    'Flush Logs',
    'Create Webhook',
    'Update Webhook',
    'Delete Webhook',
    'Disable Webhook',
];

export const createLog = async (
    event: H3Event | null,
    { action, message, system, user }: CreateLogData,
) => {
    const { logLevel } = await getSettings();
    if (logLevel === 0 || (!AllowedActions.includes(action) && logLevel !== 2)) {
        return;
    }

    const currentUser = user ?? event?.context?.user;
    const ip = event ? getClientIp(event) : '::1';
    const from = ip !== '::1' ? ` from ${ip}` : '';

    if (system || !currentUser) {
        consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - ${message}${from}`);
    } else {
        consola.info(
            `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - User ${titleCase(currentUser!.username)}, ${lowerFirst(message)}${from}`,
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
