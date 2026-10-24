import { type ChildProcessWithoutNullStreams, spawn } from 'node:child_process';
import { EventEmitter } from 'node:events';
import { existsSync } from 'node:fs';
import { join } from 'node:path';
import { createInterface } from 'node:readline';

import { app } from 'electron';
import log from 'electron-log/main';

import { t } from '../i18n';

export type ScanItem = {
    path: string;
    isFolder: boolean;
    identity: string | null;
    inSync: boolean;
    modified: boolean;
    hydrated: boolean;
    size: number;
};

export type HelperEvent =
    | { type: 'delete'; identity: string; path: string; isFolder: boolean }
    | {
          type: 'rename';
          requestId: number;
          identity: string;
          from: string;
          to: string | null;
          isFolder: boolean;
      }
    | { type: 'localNew'; path: string; isFolder: boolean; size: number }
    | { type: 'localModified'; path: string; identity: string; size: number }
    | { type: 'action'; action: string; paths: string[] }
    | { type: 'statusRequested' }
    | { type: 'rescan' };

type Pending = {
    resolve: (value: any) => void;
    reject: (error: Error) => void;
    timer: NodeJS.Timeout;
};

export const helperPath = () =>
    app.isPackaged
        ? join(process.resourcesPath, 'harbor', 'fileship-harbor.exe')
        : join(app.getAppPath(), 'build', 'harbor', 'fileship-harbor.exe');

export const isHelperAvailable = () => existsSync(helperPath());

export const harborSessionPath = () => join(process.env.LOCALAPPDATA!, 'Fileship', 'harbor.json');

export class HarborHelper extends EventEmitter<{ event: [HelperEvent]; exit: [number | null] }> {
    private child: ChildProcessWithoutNullStreams | null = null;
    private nextId = 1;
    private pending = new Map<number, Pending>();

    public start() {
        const child = spawn(helperPath(), [], { windowsHide: true });
        this.child = child;

        createInterface({ input: child.stdout }).on('line', (line) => this.handleLine(line));
        createInterface({ input: child.stderr }).on('line', (line) => log.warn('[harbor]', line));

        child.on('exit', (code) => {
            for (const pending of this.pending.values()) {
                clearTimeout(pending.timer);
                pending.reject(new Error(t('harbor.helperStopped')));
            }

            this.pending.clear();
            if (this.child === child) this.child = null;
            this.emit('exit', code);
        });
    }

    public get isRunning() {
        return !!this.child;
    }

    public request<T = Record<string, unknown>>(
        type: string,
        payload: Record<string, unknown> = {},
        timeout = 120_000,
    ) {
        const child = this.child;
        if (!child) return Promise.reject(new Error(t('harbor.helperNotRunning')));

        const id = this.nextId++;

        return new Promise<T>((resolve, reject) => {
            const timer = setTimeout(() => {
                this.pending.delete(id);
                reject(new Error(t('harbor.helperNoAnswer', { request: type })));
            }, timeout);

            this.pending.set(id, { resolve, reject, timer });
            child.stdin.write(`${JSON.stringify({ id, type, ...payload })}\n`);
        });
    }

    public ack(requestId: number, ok: boolean) {
        this.child?.stdin.write(`${JSON.stringify({ type: 'ack', requestId, ok })}\n`);
    }

    public async stop() {
        const child = this.child;
        if (!child) return;

        await this.request('stop', {}, 5000).catch(() => null);
        if (!child.killed && child.exitCode === null) child.kill();
        this.child = null;
    }

    private handleLine(line: string) {
        let message: any;

        try {
            message = JSON.parse(line);
        } catch {
            log.warn('[harbor] invalid message', line);
            return;
        }

        if (message.type !== 'result') {
            this.emit('event', message as HelperEvent);
            return;
        }

        const pending = this.pending.get(message.id);
        if (!pending) return;

        this.pending.delete(message.id);
        clearTimeout(pending.timer);

        if (message.ok) {
            pending.resolve(message);
        } else {
            pending.reject(new Error(message.error || t('harbor.helperFailed')));
        }
    }
}
