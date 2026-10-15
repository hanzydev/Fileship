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
        const token =
            socket.request.headers.cookie?.match(/(?:^|;\s*)token=([^;]+)/)?.[1] ||
            socket.handshake.auth?.token ||
            socket.request.headers.authorization?.replace(/^Bearer /, '');
        const session = token && (await findSession(token));

        if (!session) return next(new Error('Unauthorized'));

        socket.data.user = getSessionUser(session);
        next();
    });

    io.on('connection', (socket) => {
        const user = socket.data.user;

        socket.join(user.id);

        socket.on('disconnect', () => socket.leave(user.id));
        socket.on('client:ready', () =>
            socket.emit('currentUser:update', { backupRestoreState: user.backupRestoreState }),
        );
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

    nitroApp.hooks.hook('close', () => io.close());

    consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Socket server ready.`);
});
