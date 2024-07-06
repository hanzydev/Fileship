import { Server as Engine } from 'engine.io';
import { type RemoteSocket, Server } from 'socket.io';

let io: Server;

export const sendToSession = async (
    userId: string,
    sessionId: string,
    event: string,
    data: any,
) => {
    const clients = await io.in(userId).fetchSockets();
    const client = clients.find(
        (client) => client.handshake.auth.user.currentSessionId === sessionId,
    );

    return client?.emit(event, data);
};

export const sendToUser = (userId: string, event: string, data: any) =>
    io.to(userId).emit(event, data);

export const sendToAll = (event: string, data: any) => io.emit(event, data);

export const sendByFilter = async (
    filter: (socket: RemoteSocket<any, any>) => boolean,
    event: string,
    data: any,
) => {
    const clients = await io.fetchSockets();
    const filteredClients = clients.filter(filter);

    return filteredClients.map((client) => client.emit(event, data));
};

export default defineNitroPlugin((nitroApp) => {
    const engine = new Engine();
    io = new Server();

    io.bind(engine as never);

    io.use(async (socket, next) => {
        socket.handshake.auth.user = await verifyUser(
            socket.handshake.auth.sessionId,
        );

        if (!socket.handshake.auth.user) {
            return next(new Error('Unauthorized'));
        }

        next();
    });

    io.on('connection', (socket) => {
        const user = socket.handshake.auth.user;
        socket.join(user.id);

        socket.on('disconnect', () => {
            socket.leave(user.id);
        });
    });

    nitroApp.router.use(
        '/socket.io/',
        defineEventHandler({
            handler(event) {
                engine.handleRequest(event.node.req as never, event.node.res);
                event._handled = true;
            },
            websocket: {
                open(peer) {
                    const nodeContext = peer.ctx.node;
                    const req = nodeContext.req;

                    // @ts-expect-error private method
                    engine.prepare(req);

                    const rawSocket = nodeContext.req.socket;
                    const websocket = nodeContext.ws;

                    // @ts-expect-error private method
                    engine.onWebSocket(req, rawSocket, websocket);
                },
            },
        }),
    );
});
