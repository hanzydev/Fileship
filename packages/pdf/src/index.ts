import { Worker } from 'node:worker_threads';

import type { PdfPreview, PdfRequest, PdfResponse } from './types';

export type * from './types';

export type PdfClientOptions = {
    timeoutMs?: number;
    idleMs?: number;
};

export type PdfClient = {
    preview: (path: string, maxSize: number) => Promise<PdfPreview | null>;
    terminate: () => Promise<void>;
};

type Pending = { resolve: (value: PdfPreview | null) => void; reject: (error: Error) => void };

export const createPdfClient = ({
    timeoutMs = 60_000,
    idleMs = 30_000,
}: PdfClientOptions = {}): PdfClient => {
    let worker: Worker | null = null;
    let current: Pending | null = null;
    let queue: Promise<unknown> = Promise.resolve();
    let idleTimer: ReturnType<typeof setTimeout> | undefined;

    const fail = (instance: Worker, error: Error) => {
        if (worker !== instance) return;

        worker = null;
        current?.reject(error);
        current = null;
    };

    const stop = async (error: Error) => {
        const instance = worker;
        if (!instance) return;

        fail(instance, error);
        await instance.terminate();
    };

    const spawn = () => {
        const instance = new Worker(new URL('./pdf-worker.mjs', import.meta.url));

        instance.on('message', (response: PdfResponse) => {
            const call = current;
            current = null;

            if (response.type === 'result') call?.resolve(response.value);
            else call?.reject(new Error(response.message));
        });

        instance.on('error', (error) => fail(instance, error as Error));
        instance.on('exit', (code) =>
            fail(instance, new Error(`The PDF worker stopped (exit code ${code})`)),
        );

        return instance;
    };

    const run = async (request: PdfRequest) => {
        clearTimeout(idleTimer);

        worker ??= spawn();
        worker.ref();

        const instance = worker;
        const timeout = setTimeout(() => stop(new Error('The PDF worker timed out')), timeoutMs);

        try {
            return await new Promise<PdfPreview | null>((resolve, reject) => {
                current = { resolve, reject };
                instance.postMessage(request);
            });
        } finally {
            clearTimeout(timeout);
            worker?.unref();
            idleTimer = setTimeout(() => stop(new Error('The PDF worker was stopped')), idleMs);
            idleTimer.unref();
        }
    };

    return {
        preview(path, maxSize) {
            const result = queue.then(() => run({ path, maxSize }));
            queue = result.catch(() => null);

            return result;
        },
        terminate: () => stop(new Error('The PDF worker was stopped')),
    };
};
