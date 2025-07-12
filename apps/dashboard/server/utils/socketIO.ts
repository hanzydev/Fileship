import type { User } from '@prisma/client';

import { getIO } from '~~/server/plugins/socketIO';

export const sendToSession = async (
    userId: string,
    sessionId: string,
    event: string,
    data: any,
) => {
    const clients = await getIO().in(userId).fetchSockets();
    const client = clients.find((client) => client.data.user.currentSessionId === sessionId);

    return client?.emit(event, data);
};

export const sendToUser = (userId: string, event: string, data: any) =>
    getIO().to(userId).emit(event, data);

export const sendToAll = (event: string, data: any) => getIO().emit(event, data);

export const sendByFilter = async (
    filter: (user: User & { currentSessionId: string }) => boolean,
    event: string,
    data: any,
) => {
    const clients = await getIO().fetchSockets();
    const filteredClients = clients.filter((client) => filter(client.data.user));

    return filteredClients.map((client) => client.emit(event, data));
};
