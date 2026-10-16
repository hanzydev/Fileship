import { promises as fsp } from 'node:fs';
import { cpus } from 'node:os';
import { dirname, join } from 'node:path';

import type { AIConfig } from './types';

import { env } from '@huggingface/transformers';

type LoadedModel = {
    value: Promise<any>;
    users: number;
    keepAliveMs?: number;
    timer?: ReturnType<typeof setTimeout>;
    dispose: (value: any) => Promise<unknown>;
};

const loadedModels = new Map<string, LoadedModel>();

let log: (message: string) => void = () => {};
let current: AIConfig;

export const setLogger = (logger: (message: string) => void) => {
    log = logger;
};

export const getConfig = () => current;

export const getSessionOptions = () => ({
    intraOpNumThreads: Math.max(1, Math.min(cpus().length, current.threads)),
    interOpNumThreads: 1,
    enableCpuMemArena: false,
    enableMemPattern: false,
});

const unloadModel = async (key: string, entry: LoadedModel) => {
    if (loadedModels.get(key) !== entry || entry.users) return;

    loadedModels.delete(key);
    await entry.dispose(await entry.value).catch(() => null);
};

export const unloadModels = async () => {
    await Promise.all(
        [...loadedModels.entries()].map(([key, entry]) => {
            clearTimeout(entry.timer);
            return entry.users ? null : unloadModel(key, entry);
        }),
    );
};

const configKey = (config: AIConfig) =>
    JSON.stringify([config.preset, config.threads, config.cacheDir]);

export const applyConfig = async (config: AIConfig) => {
    const isChanged = current && configKey(current) !== configKey(config);

    current = config;
    env.allowLocalModels = true;
    env.cacheDir = config.cacheDir;
    env.backends.onnx.logLevel = 'error';

    if (isChanged) await unloadModels();
};

export const downloadModelFile = async (url: string, name: string) => {
    const path = join(current.cacheDir, name);
    if (await fsp.stat(path).catch(() => null)) return path;

    log(`Downloading ${name}...`);

    const response = await fetch(url);
    if (!response.ok) throw new Error(`Could not download ${name} (${response.status})`);

    await fsp.mkdir(dirname(path), { recursive: true });
    await fsp.writeFile(`${path}.tmp`, Buffer.from(await response.arrayBuffer()));
    await fsp.rename(`${path}.tmp`, path);

    return path;
};

export const useAIModel = async <T, R>(
    key: string,
    load: () => Promise<T>,
    dispose: (value: T) => Promise<unknown>,
    fn: (value: T) => Promise<R>,
    keepAliveMs?: number,
): Promise<R> => {
    let entry = loadedModels.get(key);

    if (!entry) {
        log(`Loading ${key}...`);

        entry = { value: load(), users: 0, keepAliveMs, dispose };
        loadedModels.set(key, entry);

        const loading = entry;
        entry.value.catch(() => loadedModels.get(key) === loading && loadedModels.delete(key));
    }

    clearTimeout(entry.timer);
    if (keepAliveMs !== undefined) entry.keepAliveMs = keepAliveMs;
    entry.users++;

    try {
        return await fn(await entry.value);
    } finally {
        const active = entry;

        if (!--active.users) {
            const idle = active.keepAliveMs ?? current.preset.keepAliveMs;

            if (idle) active.timer = setTimeout(() => unloadModel(key, active), idle);
            else await unloadModel(key, active);
        }
    }
};
