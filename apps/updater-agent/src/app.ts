import { timingSafeEqual } from 'node:crypto';

import {
    createApp,
    createError,
    createRouter,
    defineEventHandler,
    type EventHandler,
    getRequestHeader,
} from 'h3';

type Routes = {
    status: EventHandler;
    update: EventHandler;
};

const isSecret = (value: string | undefined, secret: string) => {
    if (!secret || !value) return false;

    const expected = Buffer.from(secret);
    const received = Buffer.from(value);

    return received.length === expected.length && timingSafeEqual(received, expected);
};

export const createAgentApp = (secret: string, routes: Routes) => {
    const app = createApp();
    const router = createRouter();

    app.use(
        defineEventHandler((event) => {
            if (!isSecret(getRequestHeader(event, 'Authorization'), secret)) {
                throw createError({ status: 401, statusText: 'Unauthorized' });
            }
        }),
    );

    app.use(router);

    router.post('/api/update', routes.update);
    router.get('/api/status', routes.status);

    return app;
};
