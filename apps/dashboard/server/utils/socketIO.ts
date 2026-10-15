import type { UserModel } from '#shared/prisma/models';
import { getIO } from '~~/server/plugins/socketIO';

export const sendToUser = (userId: string, event: string, data: any) =>
    getIO().to(userId).emit(event, data);

export const sendToAll = (event: string, data: any) => getIO().emit(event, data);

export const sendByFilter = async (
    filter: (user: UserModel & { currentSessionId: string }) => boolean,
    event: string,
    data: any,
) => {
    const clients = await getIO().fetchSockets();
    const filteredClients = clients.filter((client) => filter(client.data.user));

    return filteredClients.map((client) => client.emit(event, data));
};

export const logoutSockets = async (
    filter: (user: UserModel & { currentSessionId: string }) => boolean,
) => {
    const clients = (await getIO().fetchSockets()).filter((client) => filter(client.data.user));

    clients.forEach((client) => client.emit('logout', null));
    setTimeout(() => clients.forEach((client) => client.disconnect(true)), 1_000);
};

export const refreshSocketUser = async (userId: string, data: Partial<UserModel>) => {
    const clients = await getIO().in(userId).fetchSockets();
    clients.forEach((client) => Object.assign(client.data.user, data));
};
