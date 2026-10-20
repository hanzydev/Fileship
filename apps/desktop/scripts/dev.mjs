import { spawn, spawnSync } from 'node:child_process';
import { existsSync, watch } from 'node:fs';
import { createRequire } from 'node:module';
import { setTimeout as sleep } from 'node:timers/promises';

const require = createRequire(import.meta.url);
const electronPath = require('electron');

const RENDERER_URL = 'http://localhost:3020';
const MAIN = 'dist-electron/main.cjs';

const run = (command, args) =>
    spawn(command, args, { stdio: 'inherit', shell: process.platform === 'win32' });

const addon = spawnSync(process.execPath, ['scripts/native.mjs'], { stdio: 'inherit' });
if (addon.status !== 0) process.exit(addon.status ?? 1);

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

let electron = null;
let restartTimer = null;

const startElectron = () => {
    const args = ['.', ...process.argv.slice(2).filter((arg) => arg !== '--')];
    if (process.env.DESKTOP_DEBUG_PORT) {
        args.push(`--remote-debugging-port=${process.env.DESKTOP_DEBUG_PORT}`);
    }

    electron = spawn(electronPath, args, {
        stdio: 'inherit',
        env: { ...process.env, DESKTOP_RENDERER_URL: RENDERER_URL },
    });

    electron.on('exit', (code, signal) => {
        if (signal !== 'SIGTERM') shutdown(code ?? 0);
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
    process.exit(code);
};

process.on('SIGINT', () => shutdown(0));

await waitForRenderer();
startElectron();

watch('dist-electron', (_, file) => {
    if (file?.endsWith('.cjs')) restartElectron();
});
