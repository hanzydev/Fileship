import { execFileSync, spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { connect, type Socket } from 'node:net';
import { join } from 'node:path';
import { createInterface } from 'node:readline';

import { app } from 'electron';
import log from 'electron-log/main';

import type { HostEvents, HostInvoke } from '../shared/ipc';

type Pending = { resolve: (value: unknown) => void; reject: (error: Error) => void };

type Listener = (payload: unknown) => void;

const CONNECT_TIMEOUT_MS = 10_000;
const RECONNECT_TIMEOUT_MS = 5_000;
const RETRY_MS = 200;

const pending = new Map<number, Pending>();
const listeners = new Map<string, Set<Listener>>();

let socket: Socket | null = null;
let connecting: Promise<void> | null = null;
let nextId = 1;
let onLost: (() => void) | null = null;

export class HostError extends Error {
    public constructor(
        message: string,
        public data: Record<string, unknown> | null = null,
    ) {
        super(message);
    }
}

const argumentValue = (name: string) => {
    const index = process.argv.indexOf(name);
    return index === -1 ? null : (process.argv[index + 1] ?? null);
};

const currentUserSid = () => {
    const output = execFileSync('whoami', ['/user', '/fo', 'csv', '/nh'], {
        encoding: 'utf8',
        windowsHide: true,
    });

    return output.trim().split(',').at(-1)!.replaceAll('"', '');
};

const pipeName = () =>
    argumentValue('--host-pipe') ?? `\\\\.\\pipe\\fileship-host-${currentUserSid()}`;

const hostPath = () =>
    app.isPackaged
        ? join(process.resourcesPath, 'host', 'fileship-host.exe')
        : (process.env.DESKTOP_HOST_PATH ??
          join(app.getAppPath(), 'build', 'host', 'fileship-host.exe'));

const startHost = () => {
    const path = hostPath();
    if (!existsSync(path)) throw new Error(`The Fileship host is missing at ${path}`);

    spawn(path, ['--hidden'], { detached: true, stdio: 'ignore', windowsHide: true }).unref();
};

const handleMessage = (line: string) => {
    let message: any;

    try {
        message = JSON.parse(line);
    } catch {
        log.warn('Received an invalid message from the host', line);
        return;
    }

    if (typeof message.event === 'string') {
        for (const listener of listeners.get(message.event) ?? []) listener(message.payload);
        return;
    }

    const request = pending.get(message.id);
    if (!request) return;

    pending.delete(message.id);
    if (message.ok) request.resolve(message.result);
    else
        request.reject(
            new HostError(message.error ?? 'The host request failed', message.data ?? null),
        );
};

const handleClose = () => {
    socket = null;

    for (const request of pending.values()) request.reject(new Error('The Fileship host closed'));
    pending.clear();

    reconnect().catch(() => onLost?.());
};

const open = (name: string) =>
    new Promise<Socket>((resolve, reject) => {
        const candidate = connect(name);

        candidate.once('connect', () => {
            candidate.removeListener('error', reject);
            resolve(candidate);
        });
        candidate.once('error', reject);
    });

const attach = (connection: Socket) => {
    socket = connection;
    connection.setEncoding('utf8');
    connection.on('error', (error) => log.warn('The host connection failed', error));
    connection.on('close', handleClose);
    createInterface({ input: connection })
        .on('line', handleMessage)
        .on('error', () => null);
};

const connectWithin = async (timeout: number, start: boolean) => {
    const name = pipeName();
    const deadline = Date.now() + timeout;
    let started = !start;

    while (true) {
        try {
            attach(await open(name));
            await invokeHost('client:hello', { pid: process.pid });
            return;
        } catch (error: any) {
            if (!started && error?.code === 'ENOENT') {
                startHost();
                started = true;
            }

            if (Date.now() > deadline) throw error;
            await new Promise((resolve) => setTimeout(resolve, RETRY_MS));
        }
    }
};

const reconnect = () => connectWithin(RECONNECT_TIMEOUT_MS, false);

export const connectHost = (handleLost: () => void) => {
    onLost = handleLost;
    connecting ??= connectWithin(CONNECT_TIMEOUT_MS, !argumentValue('--host-pipe')).finally(() => {
        connecting = null;
    });

    return connecting;
};

export const invokeHost = <K extends keyof HostInvoke>(
    channel: K,
    ...args: Parameters<HostInvoke[K]>
): Promise<ReturnType<HostInvoke[K]>> => {
    if (!socket) return Promise.reject(new Error('The Fileship host is not running'));

    const id = nextId++;
    const connection = socket;

    return new Promise((resolve, reject) => {
        pending.set(id, { resolve: resolve as (value: unknown) => void, reject });
        connection.write(`${JSON.stringify({ id, channel, args })}\n`);
    });
};

export const onHost = <K extends keyof HostEvents>(
    event: K,
    listener: (payload: HostEvents[K]) => void,
) => {
    const set = listeners.get(event) ?? new Set();
    set.add(listener as Listener);
    listeners.set(event, set);
};
