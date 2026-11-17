import { defineEventHandler, toWebHandler } from 'h3';
import { beforeEach, describe, expect, it, vi } from 'vitest';

import { createAgentApp } from './app';

const SECRET = 'a7f3c9e1b5d2';

const status = vi.fn(() => ({ hasUpdate: false }));
const update = vi.fn(() => 'started');

const send = (secret: string, method: string, path: string, authorization?: string) =>
    toWebHandler(
        createAgentApp(secret, {
            status: defineEventHandler(status),
            update: defineEventHandler(update),
        }),
    )(
        new Request(`http://updater-agent:3001${path}`, {
            method,
            headers: authorization === undefined ? {} : { Authorization: authorization },
        }),
    );

const routes = [
    ['GET', '/api/status'],
    ['POST', '/api/update'],
] as const;

beforeEach(() => {
    status.mockClear();
    update.mockClear();
});

describe('createAgentApp', () => {
    it.each(routes)('refuses %s %s without the secret', async (method, path) => {
        for (const authorization of [
            undefined,
            '',
            'wrong',
            SECRET.slice(0, -1),
            `${SECRET}x`,
            `Bearer ${SECRET}`,
        ]) {
            const response = await send(SECRET, method, path, authorization);
            expect(response.status, `Authorization: ${authorization}`).toBe(401);
        }

        expect(status).not.toHaveBeenCalled();
        expect(update).not.toHaveBeenCalled();
    });

    it.each(routes)('refuses %s %s while no secret is set', async (method, path) => {
        for (const authorization of [undefined, '']) {
            expect((await send('', method, path, authorization)).status).toBe(401);
        }

        expect(status).not.toHaveBeenCalled();
        expect(update).not.toHaveBeenCalled();
    });

    it('refuses requests with a trailing slash, a query or an unknown path', async () => {
        for (const path of ['/api/status/', '/api/status?x=1', '/api/nothing', '/']) {
            expect((await send(SECRET, 'GET', path)).status, path).toBe(401);
        }

        expect(status).not.toHaveBeenCalled();
    });

    it('answers the status with the secret', async () => {
        const response = await send(SECRET, 'GET', '/api/status', SECRET);

        expect(response.status).toBe(200);
        expect(await response.json()).toEqual({ hasUpdate: false });
        expect(status).toHaveBeenCalledOnce();
    });

    it('starts an update with the secret', async () => {
        const response = await send(SECRET, 'POST', '/api/update', SECRET);

        expect(response.status).toBe(200);
        expect(await response.text()).toBe('started');
        expect(update).toHaveBeenCalledOnce();
    });

    it('keeps 404 for unknown paths with the secret', async () => {
        expect((await send(SECRET, 'GET', '/api/nothing', SECRET)).status).toBe(404);
    });
});
