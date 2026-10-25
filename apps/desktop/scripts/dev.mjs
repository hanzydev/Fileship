import { spawn, spawnSync } from 'node:child_process';
import { existsSync, watch } from 'node:fs';
import { createRequire } from 'node:module';
import { resolve } from 'node:path';
import { setTimeout as sleep } from 'node:timers/promises';

const require = createRequire(import.meta.url);
const electronPath = require('electron');

const RENDERER_URL = 'http://localhost:3020';
const MAIN = 'dist-electron/main.cjs';
const HOST = 'build/host/fileship-host.exe';
const PIPE = `\\\\.\\pipe\\fileship-host-dev-${process.pid}`;

const run = (command, args) =>
    spawn(command, args, { stdio: 'inherit', shell: process.platform === 'win32' });

const build = (script) => {
    const result = spawnSync(process.execPath, [script], { stdio: 'inherit' });
    if (result.status !== 0) process.exit(result.status ?? 1);
};

build('scripts/native-assets.mjs');
if (!existsSync('build/harbor/fileship-harbor.exe')) build('scripts/harbor.mjs');
build('scripts/host.mjs');

const passthrough = process.argv.slice(2).filter((arg) => arg !== '--');
const renderer = run('pnpm', ['exec', 'nuxt', 'dev', '--port', '3020']);
const bundler = run('pnpm', ['exec', 'tsdown', '--watch']);

const waitForRenderer = async () => {
    for (;;) {
        const isUp = await fetch(RENDERER_URL)
            .then((response) => response.ok)
            .catch(() => false);

        if (isUp && existsSync(MAIN)) return;
        await sleep(500);
    }
};

let host = null;
let electron = null;
let restartTimer = null;

const electronArgs = () => {
    const args = ['.', '--host-pipe', PIPE, ...passthrough];
    if (process.env.DESKTOP_DEBUG_PORT) {
        args.push(`--remote-debugging-port=${process.env.DESKTOP_DEBUG_PORT}`);
    }

    return args;
};

const startHost = () => {
    const command = [electronPath, ...electronArgs()].map((arg) => `"${arg}"`).join(' ');

    host = spawn(
        resolve(HOST),
        [
            '--dev',
            '--pipe',
            PIPE,
            '--renderer-url',
            RENDERER_URL,
            '--electron',
            command,
            ...passthrough,
        ],
        { stdio: 'inherit', env: { ...process.env, DESKTOP_RENDERER_URL: RENDERER_URL } },
    );

    host.on('exit', (code) => shutdown(code ?? 0));
};

const startElectron = () => {
    electron = spawn(electronPath, electronArgs(), {
        stdio: 'inherit',
        env: { ...process.env, DESKTOP_RENDERER_URL: RENDERER_URL },
    });

    electron.on('exit', () => {
        electron = null;
    });
};

const restartElectron = () => {
    clearTimeout(restartTimer);

    restartTimer = setTimeout(() => {
        if (!electron) return startElectron();

        electron.removeAllListeners('exit');
        electron.once('exit', startElectron);
        electron.kill('SIGTERM');
    }, 300);
};

const shutdown = (code) => {
    renderer.kill();
    bundler.kill();
    electron?.kill();
    host?.kill();
    process.exit(code);
};

process.on('SIGINT', () => shutdown(0));

await waitForRenderer();
startHost();
startElectron();

watch('dist-electron', (_, file) => {
    if (file?.endsWith('.cjs')) restartElectron();
});
