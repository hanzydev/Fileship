import consola from 'consola';
import dayjs from 'dayjs';
import { Server as Engine } from 'engine.io';
import { Server } from 'socket.io';

let io: Server;

export const getIO = () => io;

export default defineNitroPlugin((nitroApp) => {
    const engine = new Engine();
    io = new Server();

    io.bind(engine as never);

    io.use(async (socket, next) => {
        socket.data.user = await verifyUser(socket.handshake.auth.sessionId);

        if (!socket.data.user) {
            return next(new Error('Unauthorized'));
        }

        next();
    });

    io.on('connection', (socket) => {
        const user = socket.data.user;
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
                    // @ts-expect-error private method and property
                    engine.prepare(peer._internal.nodeReq);

                    // @ts-expect-error private method and property
                    engine.onWebSocket(
                        // @ts-expect-error private property
                        peer._internal.nodeReq,
                        // @ts-expect-error private property
                        peer._internal.nodeReq.socket,
                        peer.websocket,
                    );
                },
            },
        }),
    );

    consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Socket server ready`);
});
