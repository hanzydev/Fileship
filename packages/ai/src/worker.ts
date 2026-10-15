import { parentPort } from 'node:worker_threads';

import { methods } from './engine';
import { applyConfig, setLogger } from './models';
import type { AIRequest, AIResponse } from './types';

const port = parentPort!;
const send = (response: AIResponse) => port.postMessage(response);

setLogger((message) => send({ type: 'log', message }));

port.on('message', async ({ id, method, config, args }: AIRequest) => {
    try {
        await applyConfig(config);
        const value = await (methods[method] as (...args: unknown[]) => Promise<unknown>)(...args);

        send({ type: 'result', id, value });
    } catch (error: any) {
        send({ type: 'error', id, message: error?.message ?? String(error) });
    }
});
