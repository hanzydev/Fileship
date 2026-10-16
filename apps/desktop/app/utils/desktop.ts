import type { IpcInvoke } from '../../shared/ipc';

export const desktop = <K extends keyof IpcInvoke>(
    channel: K,
    ...args: Parameters<IpcInvoke[K]>
): Promise<ReturnType<IpcInvoke[K]>> =>
    window.fileship.invoke(channel, ...args).catch((error: Error) => {
        throw new Error(
            error.message.replace(/^Error invoking remote method '[^']+': (Error: )?/, ''),
        );
    });
