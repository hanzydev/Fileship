import { execFile } from 'node:child_process';
import { resolve } from 'node:path';
import { promisify } from 'node:util';

import { app } from 'electron';
import log from 'electron-log/main';

import { uploadPaths } from './actions';
import { t } from './i18n';

const run = promisify(execFile);

const UPLOAD_KEY = 'HKCU\\Software\\Classes\\*\\shell\\FileshipUpload';
const BATCH_MS = 400;

let pendingUploads: string[] = [];
let uploadTimer: NodeJS.Timeout | undefined;

const setValue = (key: string, name: string | null, data: string) =>
    run('reg', ['add', key, ...(name ? ['/v', name] : ['/ve']), '/t', 'REG_SZ', '/d', data, '/f']);

export const registerShellMenu = async () => {
    if (!app.isPackaged) return;

    const exe = process.execPath;

    try {
        await setValue(UPLOAD_KEY, 'MUIVerb', t('shell.upload'));
        await setValue(UPLOAD_KEY, 'Icon', exe);
        await setValue(UPLOAD_KEY, 'MultiSelectModel', 'Player');
        await setValue(`${UPLOAD_KEY}\\command`, null, `"${exe}" --upload "%1"`);
    } catch (error) {
        log.error('Shell menu registration failed', error);
    }
};

export const launchArgs = (argv: string[], name: string) => {
    const index = argv.indexOf(name);
    return index === -1 ? null : argv.slice(index + 1).filter((arg) => !arg.startsWith('-'));
};

const queueUploads = (paths: string[]) => {
    pendingUploads.push(...paths);
    clearTimeout(uploadTimer);

    uploadTimer = setTimeout(() => {
        const paths = pendingUploads;
        pendingUploads = [];
        if (paths.length) uploadPaths(paths);
    }, BATCH_MS);
};

export const handleLaunch = (argv: string[], cwd = process.cwd()) => {
    const upload = launchArgs(argv, '--upload');
    if (!upload) return false;

    const ignored = [process.execPath, app.getAppPath()];
    queueUploads(upload.map((arg) => resolve(cwd, arg)).filter((path) => !ignored.includes(path)));
    return true;
};
