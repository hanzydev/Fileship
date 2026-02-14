import fsp from 'node:fs/promises';
import { setTimeout as sleep } from 'node:timers/promises';

const isRetryableRenameError = (error: any) => {
    const code = error?.code ?? '';
    return code === 'EBUSY' || code === 'EPERM' || code === 'EACCES';
};

export const renameWithRetries = async (
    fromPath: string,
    toPath: string,
    options?: {
        retries?: number;
        initialDelayMs?: number;
    },
) => {
    const retries = options?.retries ?? 6;
    const initialDelayMs = options?.initialDelayMs ?? 25;

    let delay = initialDelayMs;

    for (let attempt = 0; attempt <= retries; attempt++) {
        try {
            await fsp.rename(fromPath, toPath);
            return;
        } catch (error: any) {
            if (attempt === retries || !isRetryableRenameError(error)) {
                throw error;
            }
            await sleep(delay);
            delay = Math.min(500, delay * 2);
        }
    }
};

export const moveFileRobust = async (
    fromPath: string,
    toPath: string,
    options?: {
        retries?: number;
        initialDelayMs?: number;
    },
) => {
    try {
        await renameWithRetries(fromPath, toPath, options);
    } catch (error: any) {
        if (!isRetryableRenameError(error)) throw error;

        await fsp.copyFile(fromPath, toPath);
        await fsp.unlink(fromPath).catch(() => null);
    }
};

export const readUtf8Snippet = async (filePath: string, maxBytes: number) => {
    const handle = await fsp.open(filePath, 'r');

    try {
        const buffer = Buffer.allocUnsafe(maxBytes);
        const { bytesRead } = await handle.read(buffer, 0, maxBytes, 0);
        return buffer.subarray(0, bytesRead).toString('utf8');
    } finally {
        await handle.close();
    }
};
