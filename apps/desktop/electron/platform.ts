import { execFileSync } from 'node:child_process';
import { join } from 'node:path';

const isWindows = process.platform === 'win32';

const currentUserSid = () => {
    const output = execFileSync('whoami', ['/user', '/fo', 'csv', '/nh'], {
        encoding: 'utf8',
        windowsHide: true,
    });

    return output.trim().split(',').at(-1)!.replaceAll('"', '');
};

export const platformName = isWindows ? 'Windows' : 'Linux';

export const hostFileName = isWindows ? 'fileship-host.exe' : 'fileship-host';

export const ffmpegFilters = isWindows ? [{ name: 'ffmpeg', extensions: ['exe'] }] : [];

export const defaultHostPipe = () =>
    isWindows
        ? `\\\\.\\pipe\\fileship-host-${currentUserSid()}`
        : join(process.env.XDG_RUNTIME_DIR!, 'fileship-host.sock');
