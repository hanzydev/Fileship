import { Worker } from 'node:worker_threads';

import type { AIConfig, AIMethods, AIRequest, AIResponse } from './types';

export type * from './types';

export type AIClientOptions = {
    config: () => AIConfig | Promise<AIConfig>;
    log?: (message: string) => void;
};

export type AIClient = AIMethods & { terminate: () => Promise<void> };

type Pending = { resolve: (value: any) => void; reject: (error: Error) => void };

const METHODS: (keyof AIMethods)[] = [
    'warmUp',
    'unload',
    'embedImage',
    'captionImage',
    'embedImageQuery',
    'embedVideo',
    'embedText',
    'recognizeText',
    'findPrivacyEntities',
];

export const createAIClient = ({ config, log = () => {} }: AIClientOptions): AIClient => {
    const pending = new Map<number, Pending>();

    let worker: Worker | null = null;
    let nextId = 0;

    const fail = (instance: Worker, error: Error) => {
        if (worker !== instance) return;

        worker = null;
        for (const { reject } of pending.values()) reject(error);
        pending.clear();
    };

    const spawn = () => {
        const instance = new Worker(new URL('./worker.mjs', import.meta.url));

        instance.on('message', (response: AIResponse) => {
            if (response.type === 'log') return log(response.message);

            const call = pending.get(response.id);
            if (!call) return;

            pending.delete(response.id);
            if (!pending.size) instance.unref();
            if (response.type === 'result') call.resolve(response.value);
            else call.reject(new Error(response.message));
        });

        instance.on('error', (error) => fail(instance, error as Error));
        instance.on('exit', (code) =>
            fail(instance, new Error(`The AI worker stopped (exit code ${code})`)),
        );
        instance.unref();

        return instance;
    };

    const call = async (method: keyof AIMethods, args: unknown[]) => {
        const request: AIRequest = { id: nextId++, method, config: await config(), args };
        worker ??= spawn();

        return new Promise((resolve, reject) => {
            pending.set(request.id, { resolve, reject });
            worker!.ref();
            worker!.postMessage(request);
        });
    };

    const client = Object.fromEntries(
        METHODS.map((method) => [method, (...args: unknown[]) => call(method, args)]),
    ) as AIMethods;

    return {
        ...client,
        async terminate() {
            const instance = worker;
            if (!instance) return;

            fail(instance, new Error('The AI worker was stopped'));
            await instance.terminate();
        },
    };
};
