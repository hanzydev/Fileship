import { execFile } from 'node:child_process';
import { resolve } from 'node:path';
import { promisify } from 'node:util';

import { app } from 'electron';
import log from 'electron-log/main';

import { uploadPaths } from './actions';
import { runHarborAction } from './harbor';
import { t } from './i18n';

const run = promisify(execFile);

const SHELL_KEY = 'HKCU\\Software\\Classes\\*\\shell';
const UPLOAD_KEY = `${SHELL_KEY}\\FileshipUpload`;
const BATCH_MS = 400;

const HARBOR_VERBS = [
    { name: 'FileshipHarborCopyLink', action: 'copyLink', label: 'shell.copyLink', many: true },
    { name: 'FileshipHarborOpen', action: 'open', label: 'shell.openInBrowser', many: true },
    { name: 'FileshipHarborShow', action: 'show', label: 'shell.showInFileship', many: false },
];

let harborRoot: string | null = null;
let pendingUploads: string[] = [];
let uploadTimer: NodeJS.Timeout | undefined;
const pendingActions = new Map<string, string[]>();
const actionTimers = new Map<string, NodeJS.Timeout>();

const setValue = (key: string, name: string | null, data: string) =>
    run('reg', ['add', key, ...(name ? ['/v', name] : ['/ve']), '/t', 'REG_SZ', '/d', data, '/f']);

const deleteKey = (key: string, name?: string) =>
    run('reg', ['delete', key, ...(name ? ['/v', name] : []), '/f']).catch(() => null);

const insideRoot = (root: string) => `System.ItemPathDisplay:~<"${root.replace(/\\+$/, '')}\\"`;

const writeHarborVerbs = async (exe: string, root: string) => {
    for (const verb of HARBOR_VERBS) {
        const key = `${SHELL_KEY}\\${verb.name}`;

        await setValue(key, 'MUIVerb', t(verb.label));
        await setValue(key, 'Icon', exe);
        await setValue(key, 'AppliesTo', insideRoot(root));
        await setValue(key, 'MultiSelectModel', verb.many ? 'Player' : 'Single');
        await setValue(`${key}\\command`, null, `"${exe}" --harbor-action ${verb.action} "%1"`);
    }
};

export const registerShellMenu = async () => {
    if (!app.isPackaged) return;

    const exe = process.execPath;

    try {
        await setValue(UPLOAD_KEY, 'MUIVerb', t('shell.upload'));
        await setValue(UPLOAD_KEY, 'Icon', exe);
        await setValue(UPLOAD_KEY, 'MultiSelectModel', 'Player');
        await setValue(`${UPLOAD_KEY}\\command`, null, `"${exe}" --upload "%1"`);

        if (harborRoot) {
            await setValue(UPLOAD_KEY, 'AppliesTo', `NOT ${insideRoot(harborRoot)}`);
            await writeHarborVerbs(exe, harborRoot);
        } else {
            await deleteKey(UPLOAD_KEY, 'AppliesTo');
        }
    } catch (error) {
        log.error('Shell menu registration failed', error);
    }
};

export const setHarborRoot = async (root: string | null) => {
    harborRoot = root;
    if (!app.isPackaged) return;

    if (!root) {
        for (const verb of HARBOR_VERBS) await deleteKey(`${SHELL_KEY}\\${verb.name}`);
    }

    await registerShellMenu();
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

const queueHarborAction = (action: string, paths: string[]) => {
    pendingActions.set(action, [...(pendingActions.get(action) ?? []), ...paths]);
    clearTimeout(actionTimers.get(action));

    actionTimers.set(
        action,
        setTimeout(() => {
            const paths = pendingActions.get(action)!;
            pendingActions.delete(action);
            actionTimers.delete(action);
            runHarborAction(action, paths);
        }, BATCH_MS),
    );
};

export const handleLaunch = (argv: string[], cwd = process.cwd()) => {
    const ignored = [process.execPath, app.getAppPath()];
    const resolvePaths = (args: string[]) =>
        args.map((arg) => resolve(cwd, arg)).filter((path) => !ignored.includes(path));

    const [action, ...targets] = launchArgs(argv, '--harbor-action') ?? [];

    if (action) {
        queueHarborAction(action, resolvePaths(targets));
        return true;
    }

    const upload = launchArgs(argv, '--upload');
    if (!upload) return false;

    queueUploads(resolvePaths(upload));
    return true;
};
