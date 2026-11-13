import { type ChildProcess, spawn } from 'node:child_process';
import { existsSync } from 'node:fs';

import consola from 'consola';
import { join } from 'pathe';

const HEALTH_INTERVAL_MS = 10_000;
const RESTART_DELAY_MS = 5_000;

const executable = join(
    rootDirectory,
    'apps/ml/.venv',
    process.platform === 'win32' ? 'Scripts/fileship-ml.exe' : 'bin/fileship-ml',
);

export default defineNitroPlugin((nitroApp) => {
    let child: ChildProcess | null = null;
    let closing = false;

    const start = () => {
        child = spawn(executable, {
            env: {
                ...process.env,
                FILESHIP_ML_HOST: '127.0.0.1',
                FILESHIP_ML_PORT: '3003',
                FILESHIP_ML_MODELS_DIR: join(dataDirectory, 'models'),
            },
            stdio: ['ignore', 'inherit', 'inherit'],
        });

        child.on('error', (error) =>
            consola.error(`Starting the AI service failed: ${error.message}`),
        );
        child.on('exit', () => !closing && setTimeout(start, RESTART_DELAY_MS));
    };

    const check = async () => {
        const health = await $fetch<{ modelsBytes: number }>('/health', {
            baseURL: aiService.url,
            timeout: 5_000,
        }).catch(() => null);

        const wasUp = aiService.up;

        aiService.up = !!health;
        if (!health) return;

        aiService.modelsBytes = health.modelsBytes;

        if (!wasUp && (await isAiEnabled())) {
            ai.warmUp().catch((error) =>
                consola.error(`Loading AI models failed: ${error?.message ?? error}`),
            );
        }
    };

    if (aiService.managed && existsSync(executable)) start();

    check();
    const timer = setInterval(check, HEALTH_INTERVAL_MS);

    nitroApp.hooks.hook('close', () => {
        closing = true;
        clearInterval(timer);
        child?.kill();
    });
});
